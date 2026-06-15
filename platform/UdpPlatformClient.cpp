#include "UdpPlatformClient.h"
#include <QHostAddress>
#include <QDateTime>
#include <QtEndian>
#include <cstring>

UdpPlatformClient::UdpPlatformClient(const PlatformConfig& cfg, QObject* parent)
    : QObject(parent)
    , m_cfg(cfg)
{}

bool UdpPlatformClient::bindSockets()
{
    // 命令套接字：绑定任意本地端口
    m_cmdSocket = new QUdpSocket(this);
    if (!m_cmdSocket->bind(QHostAddress::AnyIPv4, 0)) {
        emit errorOccurred(QString("UDP cmd socket bind failed: %1")
                           .arg(m_cmdSocket->errorString()));
        return false;
    }

    // 推送接收套接字：绑定本地 pushPort
    m_pushSocket = new QUdpSocket(this);
    if (!m_pushSocket->bind(QHostAddress::AnyIPv4, m_cfg.pushPort,
                            QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        emit errorOccurred(QString("UDP push socket bind failed on port %1: %2")
                           .arg(m_cfg.pushPort).arg(m_pushSocket->errorString()));
        return false;
    }
    connect(m_pushSocket, &QUdpSocket::readyRead,
            this, &UdpPlatformClient::onPushDataReady);
    return true;
}

void UdpPlatformClient::close()
{
    if (m_cmdSocket)  { m_cmdSocket->close();  }
    if (m_pushSocket) { m_pushSocket->close(); }
}

bool UdpPlatformClient::writeValue(quint32 funcCode, double value)
{
    return writeValues({{funcCode, value}});
}

bool UdpPlatformClient::writeValues(const QVector<QPair<quint32, double>>& entries)
{
    if (!m_cmdSocket) return false;
    QByteArray frame = buildWriteFrame(entries);
    qint64 sent = m_cmdSocket->writeDatagram(
        frame, QHostAddress(m_cfg.ip), m_cfg.cmdPort);
    return (sent == frame.size());
}

// -----------------------------------------------------------------------
// 帧构建（写命令，500 字节，小端序）
// -----------------------------------------------------------------------
QByteArray UdpPlatformClient::buildWriteFrame(const QVector<QPair<quint32, double>>& entries)
{
    QByteArray frame(CMD_FRAME_SIZE, 0x00);
    quint8* buf = reinterpret_cast<quint8*>(frame.data());

    buf[0] = 0xAA;
    buf[1] = 0x55;

    QDateTime now = QDateTime::currentDateTime();
    auto writeU16 = [&](int offset, quint16 val) {
        qToLittleEndian(val, &buf[offset]);
    };
    writeU16(2,  static_cast<quint16>(now.date().year()));
    writeU16(4,  static_cast<quint16>(now.date().month()));
    writeU16(6,  static_cast<quint16>(now.date().day()));
    writeU16(8,  static_cast<quint16>(now.time().hour()));
    writeU16(10, static_cast<quint16>(now.time().minute()));
    writeU16(12, static_cast<quint16>(now.time().second()));
    writeU16(14, static_cast<quint16>(now.time().msec()));

    writeU16(16, ++m_seqNo);
    writeU16(18, REQ_WRITE);
    int count = qMin(entries.size(), MAX_ENTRIES_PER_FRAME);
    writeU16(20, static_cast<quint16>(count));

    int offset = 22;
    for (int i = 0; i < count; ++i) {
        quint32 fc  = entries[i].first;
        double  val = entries[i].second;

        qToLittleEndian(static_cast<quint16>(fc & 0xFFFF), &buf[offset]);
        offset += 2;
        qToLittleEndian(static_cast<quint16>(fc >> 16), &buf[offset]);
        offset += 2;
        std::memcpy(&buf[offset], &val, sizeof(double));
        offset += 8;
    }

    buf[498] = 0x66;
    buf[499] = 0xBB;

    return frame;
}

// -----------------------------------------------------------------------
// 解析实时推送帧（1578 字节）
// -----------------------------------------------------------------------
void UdpPlatformClient::onPushDataReady()
{
    while (m_pushSocket->hasPendingDatagrams()) {
        QByteArray data;
        data.resize(static_cast<int>(m_pushSocket->pendingDatagramSize()));
        m_pushSocket->readDatagram(data.data(), data.size());
        if (data.size() == PUSH_FRAME_SIZE)
            parsePushFrame(data);
    }
}

void UdpPlatformClient::parsePushFrame(const QByteArray& data)
{
    const quint8* buf = reinterpret_cast<const quint8*>(data.constData());

    if (buf[0] != 0xAA || buf[1] != 0x55) return;
    if (buf[1576] != 0x66 || buf[1577] != 0xBB) return;

    PlatformState state;
    state.valid = true;

    int dataIdx = 0;
    for (int i = 0; i < 180 && dataIdx < 150; ++i) {
        quint16 fc = qFromLittleEndian<quint16>(&buf[16 + i * 2]);
        if (fc == 0) continue;

        double val;
        std::memcpy(&val, &buf[376 + dataIdx * 8], sizeof(double));
        ++dataIdx;

        switch (fc) {
        case static_cast<quint16>(FuncCode::DEVICE_STATE & 0xFFFF):
            state.deviceStatus = static_cast<quint32>(val);
            state.powered      = (state.deviceStatus & 0x01) != 0;
            state.faultActive  = (state.deviceStatus & 0x04) != 0;
            state.motionMode   = static_cast<int>((state.deviceStatus >> 8) & 0xFF);
            break;
        case static_cast<quint16>(FuncCode::POSE_X  & 0xFFFF): state.currentPose.x  = val; break;
        case static_cast<quint16>(FuncCode::POSE_Y  & 0xFFFF): state.currentPose.y  = val; break;
        case static_cast<quint16>(FuncCode::POSE_Z  & 0xFFFF): state.currentPose.z  = val; break;
        case static_cast<quint16>(FuncCode::POSE_RX & 0xFFFF): state.currentPose.rx = val; break;
        case static_cast<quint16>(FuncCode::POSE_RY & 0xFFFF): state.currentPose.ry = val; break;
        case static_cast<quint16>(FuncCode::POSE_RZ & 0xFFFF): state.currentPose.rz = val; break;
        default:
            if (fc >= static_cast<quint16>(FuncCode::CHAIN_BASE & 0xFFFF) &&
                fc <  static_cast<quint16>((FuncCode::CHAIN_BASE + 6) & 0xFFFF)) {
                int ch = fc - static_cast<quint16>(FuncCode::CHAIN_BASE & 0xFFFF);
                if (ch < 6) state.chainLengths[ch] = val;
            } else if (fc >= static_cast<quint16>(FuncCode::EXT_SENSOR_BASE & 0xFFFF) &&
                       fc <  static_cast<quint16>((FuncCode::EXT_SENSOR_BASE + 6) & 0xFFFF)) {
                int ch = fc - static_cast<quint16>(FuncCode::EXT_SENSOR_BASE & 0xFFFF);
                if (ch < 6) state.extSensors[ch] = val;
            }
            break;
        }
    }

    emit platformStateReceived(state);
}
