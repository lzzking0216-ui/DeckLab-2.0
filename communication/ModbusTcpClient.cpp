#include "ModbusTcpClient.h"

ModbusTcpClient::ModbusTcpClient(const PlcConfig& cfg, QObject* parent)
    : QObject(parent)
    , m_cfg(cfg)
{
    m_socket = new QTcpSocket(this);
}

ModbusTcpClient::~ModbusTcpClient()
{
    disconnectFromPlc();
}

bool ModbusTcpClient::connectToPlc()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState)
        return true;

    m_socket->connectToHost(m_cfg.ip, m_cfg.port);
    if (!m_socket->waitForConnected(m_cfg.connectTimeoutMs)) {
        emit errorOccurred(QString("连接失败: %1").arg(m_socket->errorString()));
        return false;
    }
    emit connectionStateChanged(true);
    return true;
}

void ModbusTcpClient::disconnectFromPlc()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
        m_socket->waitForDisconnected(1000);
        emit connectionStateChanged(false);
    }
}

bool ModbusTcpClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

// -----------------------------------------------------------------------
// MBAP 头构建（Transaction ID + Protocol 0 + Length + Unit ID）
// -----------------------------------------------------------------------

// FC 0x03: Read Holding Registers
QByteArray ModbusTcpClient::buildReadRequest(quint16 startAddr, quint16 count)
{
    quint16 tid = nextTransactionId();
    QByteArray buf(12, 0);
    buf[0] = (tid >> 8) & 0xFF;  buf[1] = tid & 0xFF;       // Transaction ID
    buf[2] = 0;                  buf[3] = 0;                  // Protocol ID
    buf[4] = 0;                  buf[5] = 6;                  // Length = 6
    buf[6] = m_cfg.unitId;                                    // Unit ID
    buf[7] = 0x03;                                            // Function code
    buf[8] = (startAddr >> 8) & 0xFF; buf[9]  = startAddr & 0xFF;
    buf[10]= (count >> 8)     & 0xFF; buf[11] = count     & 0xFF;
    return buf;
}

// FC 0x06: Write Single Register
QByteArray ModbusTcpClient::buildWriteSingleRequest(quint16 addr, quint16 value)
{
    quint16 tid = nextTransactionId();
    QByteArray buf(12, 0);
    buf[0] = (tid >> 8) & 0xFF;  buf[1] = tid & 0xFF;
    buf[2] = 0;                  buf[3] = 0;
    buf[4] = 0;                  buf[5] = 6;
    buf[6] = m_cfg.unitId;
    buf[7] = 0x06;
    buf[8] = (addr  >> 8) & 0xFF; buf[9]  = addr  & 0xFF;
    buf[10]= (value >> 8) & 0xFF; buf[11] = value & 0xFF;
    return buf;
}

// FC 0x10: Write Multiple Registers
QByteArray ModbusTcpClient::buildWriteMultipleRequest(quint16 startAddr, const QVector<quint16>& values)
{
    quint16 tid  = nextTransactionId();
    quint16 qty  = static_cast<quint16>(values.size());
    quint8  byteCount = static_cast<quint8>(qty * 2);
    quint16 pduLen    = static_cast<quint16>(7 + qty * 2);

    QByteArray buf;
    buf.resize(7 + 6 + byteCount);

    buf[0] = (tid >> 8) & 0xFF; buf[1] = tid & 0xFF;
    buf[2] = 0;                 buf[3] = 0;
    buf[4] = (pduLen >> 8) & 0xFF; buf[5] = pduLen & 0xFF;
    buf[6] = m_cfg.unitId;
    buf[7] = 0x10;
    buf[8] = (startAddr >> 8) & 0xFF; buf[9]  = startAddr & 0xFF;
    buf[10]= (qty >> 8) & 0xFF;       buf[11] = qty & 0xFF;
    buf[12]= byteCount;

    for (int i = 0; i < values.size(); ++i) {
        buf[13 + i*2]     = (values[i] >> 8) & 0xFF;
        buf[13 + i*2 + 1] = values[i] & 0xFF;
    }
    return buf;
}

// -----------------------------------------------------------------------
// 阻塞式发送/接收，校验 Transaction ID 防止接受残余响应
// -----------------------------------------------------------------------
bool ModbusTcpClient::sendAndReceive(quint16 expectedTid, const QByteArray& req,
                                     QByteArray& resp, int expectedLen)
{
    // 清空 socket 残余数据（上次超时可能留下部分响应）
    m_socket->readAll();

    m_socket->write(req);
    if (!m_socket->waitForBytesWritten(m_cfg.ioTimeoutMs)) {
        emit errorOccurred("Modbus 发送超时");
        return false;
    }

    resp.clear();
    while (resp.size() < expectedLen) {
        if (!m_socket->waitForReadyRead(m_cfg.ioTimeoutMs)) {
            emit errorOccurred("Modbus 接收超时");
            return false;
        }
        resp += m_socket->readAll();
    }

    if (resp.size() < expectedLen)
        return false;

    // 校验 Transaction ID（MBAP 头前两字节）
    quint16 rxTid = (static_cast<quint8>(resp[0]) << 8) | static_cast<quint8>(resp[1]);
    if (rxTid != expectedTid) {
        emit errorOccurred(QString("Modbus TID 不匹配: 期望 %1 收到 %2").arg(expectedTid).arg(rxTid));
        return false;
    }

    // 检查异常响应（功能码最高位置1）
    if ((static_cast<quint8>(resp[7]) & 0x80)) {
        quint8 exCode = (resp.size() >= 9) ? static_cast<quint8>(resp[8]) : 0;
        emit errorOccurred(QString("Modbus 异常响应，异常码: 0x%1").arg(exCode, 2, 16, QChar('0')));
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------
// 公共读写接口
// -----------------------------------------------------------------------
bool ModbusTcpClient::readHoldingRegisters(quint16 startAddr, quint16 count, QVector<quint16>& out)
{
    if (!m_socket->isOpen()) return false;

    QByteArray req  = buildReadRequest(startAddr, count);
    quint16    tid  = (static_cast<quint8>(req[0]) << 8) | static_cast<quint8>(req[1]);
    QByteArray resp;
    int expectedLen = 9 + count * 2;
    if (!sendAndReceive(tid, req, resp, expectedLen)) return false;
    if (static_cast<quint8>(resp[7]) != 0x03) return false;

    int byteCount = static_cast<quint8>(resp[8]);
    if (byteCount != count * 2) return false;

    out.resize(count);
    for (int i = 0; i < count; ++i)
        out[i] = (static_cast<quint8>(resp[9 + i*2]) << 8) | static_cast<quint8>(resp[10 + i*2]);

    return true;
}

bool ModbusTcpClient::writeSingleRegister(quint16 addr, quint16 value)
{
    if (!m_socket->isOpen()) return false;

    QByteArray req  = buildWriteSingleRequest(addr, value);
    quint16    tid  = (static_cast<quint8>(req[0]) << 8) | static_cast<quint8>(req[1]);
    QByteArray resp;
    return sendAndReceive(tid, req, resp, 12);
}

bool ModbusTcpClient::writeMultipleRegisters(quint16 startAddr, const QVector<quint16>& values)
{
    if (!m_socket->isOpen()) return false;

    QByteArray req  = buildWriteMultipleRequest(startAddr, values);
    quint16    tid  = (static_cast<quint8>(req[0]) << 8) | static_cast<quint8>(req[1]);
    QByteArray resp;
    return sendAndReceive(tid, req, resp, 12);
}
