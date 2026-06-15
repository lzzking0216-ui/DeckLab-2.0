#include "PlcWorker.h"
#include <QThread>

PlcWorker::PlcWorker(const PlcConfig& cfg, QObject* parent)
    : QObject(parent)
    , m_cfg(cfg)
{
    for (int i = 0; i < 8; ++i)
        m_controllers.append(new MotorController(i, this));
}

void PlcWorker::initialize()
{
    m_client = new ModbusTcpClient(m_cfg, this);
    connect(m_client, &ModbusTcpClient::connectionStateChanged,
            this, &PlcWorker::connectionStateChanged);
    connect(m_client, &ModbusTcpClient::errorOccurred,
            this, &PlcWorker::errorOccurred);

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(20);  // 20ms = 50Hz
    connect(m_pollTimer, &QTimer::timeout, this, &PlcWorker::pollCycle);

    m_reconnTimer = new QTimer(this);
    m_reconnTimer->setSingleShot(false);
    connect(m_reconnTimer, &QTimer::timeout, this, &PlcWorker::tryReconnect);

    if (!m_client->connectToPlc()) {
        m_reconnTimer->setInterval(m_reconnIntervalMs);
        m_reconnTimer->start();
    } else {
        clearLocalAxesCtrlWords();
        m_pollTimer->start();
    }
}

void PlcWorker::stop()
{
    if (m_pollTimer)   m_pollTimer->stop();
    if (m_reconnTimer) m_reconnTimer->stop();
    if (m_client)      m_client->disconnectFromPlc();
}

// -----------------------------------------------------------------------
// 轮询
// -----------------------------------------------------------------------
void PlcWorker::pollCycle()
{
    if (!m_client->isConnected()) {
        m_pollTimer->stop();
        m_reconnIntervalMs = 500;
        m_reconnTimer->setInterval(m_reconnIntervalMs);
        m_reconnTimer->start();
        emit connectionStateChanged(false);
        return;
    }
    readAllMotors();
    readSensors();
}

// -----------------------------------------------------------------------
// 读取本地电机状态（轴6卷筒 + 轴7 RSD，MW60-MW79，一次读20个寄存器）
// -----------------------------------------------------------------------
void PlcWorker::readAllMotors()
{
    constexpr int     LOCAL_FIRST = 6;
    constexpr int     LOCAL_COUNT = 2;
    constexpr quint16 startAddr   = Reg::AXIS_BASE + LOCAL_FIRST * Reg::AXIS_STRIDE;

    QVector<quint16> regs;
    if (!m_client->readHoldingRegisters(startAddr, LOCAL_COUNT * Reg::AXIS_STRIDE, regs))
        return;

    for (int i = 0; i < LOCAL_COUNT; ++i) {
        int ax   = LOCAL_FIRST + i;
        int base = i * Reg::AXIS_STRIDE;

        MotorData d;
        d.axisId     = ax;
        d.statusWord = regs[base + Reg::AX_STATUS_WORD];
        d.actualVel  = static_cast<qint16>(regs[base + Reg::AX_ACT_VEL]);
        d.actualPos  = static_cast<qint32>(
                           (static_cast<quint32>(regs[base + Reg::AX_ACT_POS_H]) << 16)
                           | regs[base + Reg::AX_ACT_POS]);
        d.torque     = static_cast<qint16>(regs[base + Reg::AX_TORQUE]);
        d.errorCode  = regs[base + Reg::AX_ERROR_CODE];
        d.enabled    = (d.statusWord & 0x0001) != 0;
        d.faultActive= (d.statusWord & 0x0004) != 0;
        d.stateName  = smStateName(d.statusWord);

        m_controllers[ax]->updateFromData(d);
        emit motorDataUpdated(ax, d);
    }
}

// -----------------------------------------------------------------------
// 拉线位移传感器（BRT38 SSI，6路，MW256-MW267）
// -----------------------------------------------------------------------
void PlcWorker::readSensors()
{
    const int totalRegs = Reg::SENSOR_COUNT * Reg::SENSOR_REGS_PER_CH;
    QVector<quint16> regs;
    if (!m_client->readHoldingRegisters(Reg::SENSOR_BASE, totalRegs, regs))
        return;

    static const char* names[Reg::SENSOR_COUNT] = {
        "拉线 1", "拉线 2", "拉线 3",
        "拉线 4", "拉线 5", "拉线 6"
    };
    static const float kSpoolCircumference[Reg::SENSOR_COUNT] = {
        250.0f, 250.0f, 250.0f, 250.0f, 250.0f, 250.0f
    };
    static const float kSingleTurnRes[Reg::SENSOR_COUNT] = {
        4096.0f, 4096.0f, 4096.0f, 4096.0f, 4096.0f, 4096.0f
    };

    SensorData data;
    for (int i = 0; i < Reg::SENSOR_COUNT; ++i) {
        int base = i * Reg::SENSOR_REGS_PER_CH;
        quint32 raw = (static_cast<quint32>(regs[base + 1]) << 16)
                    |  static_cast<quint32>(regs[base]);
        qint32 counts = static_cast<qint32>(raw);

        auto& ch = data.channels[i];
        ch.name         = QString::fromUtf8(names[i]);
        ch.unit         = "mm";
        ch.rawCounts    = counts;
        ch.displayValue = static_cast<float>(counts)
                          * (kSpoolCircumference[i] / kSingleTurnRes[i]);
        ch.valid        = true;
    }
    emit sensorDataUpdated(data);
}

void PlcWorker::tryReconnect()
{
    if (m_client->connectToPlc()) {
        m_reconnIntervalMs = 500;
        m_reconnTimer->stop();
        clearLocalAxesCtrlWords();
        m_pollTimer->start();
    } else {
        m_reconnIntervalMs = qMin(m_reconnIntervalMs * 2, 10000);
        m_reconnTimer->setInterval(m_reconnIntervalMs);
    }
}

// -----------------------------------------------------------------------
// 低层写帮助
// -----------------------------------------------------------------------
void PlcWorker::writeCtrlWord(int axisId, quint16 cw)
{
    m_client->writeSingleRegister(Reg::axisAddr(axisId, Reg::AX_CTRL_WORD), cw);
}

void PlcWorker::clearLocalAxesCtrlWords()
{
    writeCtrlWord(6, 0x0000);
    writeCtrlWord(7, 0x0000);
}

void PlcWorker::writeRegister(quint16 addr, quint16 value)
{
    if (m_client) m_client->writeSingleRegister(addr, value);
}

void PlcWorker::writeRegisters(quint16 addr, QVector<quint16> values)
{
    if (m_client) m_client->writeMultipleRegisters(addr, values);
}

// -----------------------------------------------------------------------
// 轴控制命令
// -----------------------------------------------------------------------

void PlcWorker::enableAxis(int axisId)
{
    if (!m_client || !m_client->isConnected()) return;
    writeCtrlWord(axisId, 0x0001);
}

void PlcWorker::disableAxis(int axisId)
{
    if (!m_client || !m_client->isConnected()) return;
    writeCtrlWord(axisId, 0x0000);
}

void PlcWorker::faultResetAxis(int axisId)
{
    if (!m_client || !m_client->isConnected()) return;
    writeCtrlWord(axisId, 0x0000);
    QTimer::singleShot(20, this, [this, axisId] {
        writeCtrlWord(axisId, 0x0009);
        QTimer::singleShot(100, this, [this, axisId] {
            writeCtrlWord(axisId, 0x0001);
        });
    });
}

void PlcWorker::setVelocity(int axisId, double rpm)
{
    if (!m_client || !m_client->isConnected()) return;
    //if (!m_controllers[axisId]->currentData().enabled) return;  // *[调试时可注释此行]

    quint16 addr = Reg::axisAddr(axisId, Reg::AX_TARGET_VEL);
    qint16  val  = static_cast<qint16>(rpm * 10.0);
    m_client->writeSingleRegister(addr, static_cast<quint16>(val));

    writeCtrlWord(axisId, 0x0001);
    QTimer::singleShot(20, this, [this, axisId] {
        writeCtrlWord(axisId, 0x0003);
        QTimer::singleShot(20, this, [this, axisId] {
            writeCtrlWord(axisId, 0x0001);
        });
    });
}

void PlcWorker::setPosition(int axisId, qint32 pulses, double approachRpm)
{
    if (!m_client || !m_client->isConnected()) return;
    //if (!m_controllers[axisId]->currentData().enabled) return;  // *[调试时可注释此行]

    quint16 base = Reg::AXIS_BASE + static_cast<quint16>(axisId) * Reg::AXIS_STRIDE;

    qint16 rpmVal = static_cast<qint16>(approachRpm * 10.0);
    m_client->writeSingleRegister(base + Reg::AX_TARGET_VEL, static_cast<quint16>(rpmVal));

    QVector<quint16> posRegs = {
        static_cast<quint16>(static_cast<quint32>(pulses) & 0xFFFF),
        static_cast<quint16>(static_cast<quint32>(pulses) >> 16)
    };
    m_client->writeMultipleRegisters(base + Reg::AX_TARGET_POS, posRegs);

    writeCtrlWord(axisId, 0x0001);
    QTimer::singleShot(20, this, [this, axisId] {
        writeCtrlWord(axisId, 0x0003);
        QTimer::singleShot(20, this, [this, axisId] {
            writeCtrlWord(axisId, 0x0001);
        });
    });
}

void PlcWorker::emergencyStopAll()
{
    if (!m_client) return;
    for (int ax = 0; ax < 8; ++ax)
        writeCtrlWord(ax, 0x0000);
    m_client->writeSingleRegister(Reg::SYS_ESTOP_CMD, 1);
}
