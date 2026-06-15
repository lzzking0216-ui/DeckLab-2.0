# Modbus TCP 通信层 — 完整参考

## ModbusTcpClient 类

```cpp
// communication/ModbusTcpClient.h
#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QMutex>

struct PlcConfig {
    QString ip   = "192.168.1.1";
    quint16 port = 502;
    int timeoutMs = 3000;
    quint8  unitId = 1;      // Modbus 从站地址，AC702 默认为 1
};

class ModbusTcpClient : public QObject {
    Q_OBJECT
public:
    explicit ModbusTcpClient(const PlcConfig& cfg, QObject* parent = nullptr);

    bool connectToPlc();
    void disconnectFromPlc();
    bool isConnected() const;

    // 功能码 0x03 — 读保持寄存器
    bool readHoldingRegisters(quint16 startAddr, quint16 count, QVector<quint16>& out);
    // 功能码 0x01 — 读线圈
    bool readCoils(quint16 startAddr, quint16 count, QVector<bool>& out);
    // 功能码 0x06 — 写单个寄存器
    bool writeSingleRegister(quint16 addr, quint16 value);
    // 功能码 0x10 — 写多个寄存器
    bool writeMultipleRegisters(quint16 startAddr, const QVector<quint16>& values);
    // 功能码 0x05 — 写单个线圈
    bool writeSingleCoil(quint16 addr, bool value);

    // 32位有符号整数读写（占2个寄存器，高位在前）
    bool readInt32(quint16 startAddr, qint32& out);
    bool writeInt32(quint16 startAddr, qint32 value);

signals:
    void connectionStateChanged(bool connected);
    void errorOccurred(const QString& msg);

private:
    PlcConfig    m_cfg;
    QTcpSocket*  m_socket;
    quint16      m_transactionId = 0;
    mutable QMutex m_mutex;

    QByteArray  buildRequest(quint8 funcCode, quint16 startAddr, quint16 countOrValue);
    QByteArray  buildWriteMultipleRequest(quint16 startAddr, const QVector<quint16>& values);
    bool        sendAndReceive(const QByteArray& request, QByteArray& response, int expectedLen);
    quint16     nextTransactionId() { return ++m_transactionId; }
};
```

```cpp
// communication/ModbusTcpClient.cpp — 关键实现

bool ModbusTcpClient::connectToPlc() {
    m_socket->connectToHost(m_cfg.ip, m_cfg.port);
    if (!m_socket->waitForConnected(m_cfg.timeoutMs)) {
        emit errorOccurred(QString("连接失败: %1").arg(m_socket->errorString()));
        return false;
    }
    emit connectionStateChanged(true);
    return true;
}

// MBAP Header: Transaction(2) + Protocol(2) + Length(2) + UnitId(1) = 7字节
QByteArray ModbusTcpClient::buildRequest(quint8 funcCode, quint16 startAddr, quint16 count) {
    QByteArray buf(12, 0);
    quint16 tid = nextTransactionId();
    // Transaction ID
    buf[0] = (tid >> 8) & 0xFF;  buf[1] = tid & 0xFF;
    // Protocol ID = 0
    buf[2] = 0; buf[3] = 0;
    // Length = 6 (UnitId + FuncCode + StartAddr(2) + Count(2))
    buf[4] = 0; buf[5] = 6;
    buf[6] = m_cfg.unitId;
    buf[7] = funcCode;
    buf[8] = (startAddr >> 8) & 0xFF; buf[9] = startAddr & 0xFF;
    buf[10]= (count >> 8) & 0xFF;     buf[11]= count & 0xFF;
    return buf;
}

bool ModbusTcpClient::readHoldingRegisters(quint16 startAddr, quint16 count, QVector<quint16>& out) {
    QMutexLocker lk(&m_mutex);
    if (!m_socket->isOpen()) return false;

    QByteArray req = buildRequest(0x03, startAddr, count);
    QByteArray resp;
    // 期望响应长度：MBAP(7) + FuncCode(1) + ByteCount(1) + Data(count*2)
    if (!sendAndReceive(req, resp, 9 + count * 2)) return false;

    if ((quint8)resp[7] != 0x03) return false;  // 功能码校验
    int byteCount = (quint8)resp[8];
    if (byteCount != count * 2) return false;

    out.resize(count);
    for (int i = 0; i < count; ++i) {
        out[i] = ((quint8)resp[9 + i*2] << 8) | (quint8)resp[10 + i*2];
    }
    return true;
}

bool ModbusTcpClient::writeSingleRegister(quint16 addr, quint16 value) {
    QMutexLocker lk(&m_mutex);
    QByteArray req = buildRequest(0x06, addr, value);  // 复用 count 字段传 value
    QByteArray resp;
    return sendAndReceive(req, resp, 12);  // 功能码0x06响应与请求等长
}

bool ModbusTcpClient::readInt32(quint16 startAddr, qint32& out) {
    QVector<quint16> regs;
    if (!readHoldingRegisters(startAddr, 2, regs)) return false;
    // 高字在前（Big Endian Word Order），与 AC702 默认一致
    out = (qint32)((quint32)regs[0] << 16 | regs[1]);
    return true;
}
```

---

## PlcWorker — 后台轮询线程

```cpp
// communication/PlcWorker.h
#pragma once
#include <QObject>
#include <QTimer>
#include "ModbusTcpClient.h"
#include "RegisterMap.h"

class PlcWorker : public QObject {
    Q_OBJECT
public:
    explicit PlcWorker(const PlcConfig& cfg, QObject* parent = nullptr);

public slots:
    void start(int cycleMs = 20);   // 默认 20ms（50Hz）
    void stop();
    // 来自 UI 的写指令（在 Worker 线程执行，线程安全）
    void writeRegister(quint16 addr, quint16 value);
    void writeRegisters(quint16 addr, QVector<quint16> values);

signals:
    void motorDataUpdated(int axisId, MotorData data);
    void sensorDataUpdated(SensorData data);
    void connectionStateChanged(bool connected);
    void alarmTriggered(AlarmEvent ev);

private slots:
    void pollCycle();
    void tryReconnect();

private:
    ModbusTcpClient* m_client;
    QTimer*          m_pollTimer;
    QTimer*          m_reconnTimer;
    int              m_reconnIntervalMs = 500;

    void readAllMotors();     // 批量读 8 轴寄存器
    void readSensors();       // 读 GL20-RTU-ECT 传感器区
    void checkSystemAlarms(); // 检查系统报警字
};
```

```cpp
// communication/PlcWorker.cpp — 关键实现

void PlcWorker::pollCycle() {
    if (!m_client->isConnected()) return;
    readAllMotors();
    readSensors();
    checkSystemAlarms();
}

void PlcWorker::readAllMotors() {
    // 一次性读取所有8轴寄存器（减少 Modbus 往返次数）
    // 8轴 × 10寄存器/轴 = 80个寄存器
    QVector<quint16> regs;
    if (!m_client->readHoldingRegisters(Reg::AXIS_BASE, 8 * Reg::AXIS_STRIDE, regs))
        return;

    for (int ax = 0; ax < 8; ++ax) {
        MotorData d;
        int base = ax * Reg::AXIS_STRIDE;
        d.axisId     = ax;
        d.statusWord = regs[base + Reg::AX_STATUS_WORD];
        d.actualVel  = (qint16)regs[base + Reg::AX_ACT_VEL];    // 有符号
        d.actualPos  = (qint32)((quint32)regs[base + Reg::AX_ACT_POS_H] << 16
                                | regs[base + Reg::AX_ACT_POS]);
        d.torque     = (qint16)regs[base + Reg::AX_TORQUE];      // 0.1%
        d.errorCode  = regs[base + Reg::AX_ERROR_CODE];
        d.enabled    = (d.statusWord & 0x0004) != 0;             // Operation Enabled bit
        d.faultActive= (d.statusWord & 0x0008) != 0;             // Fault bit
        emit motorDataUpdated(ax, d);
    }
}

void PlcWorker::tryReconnect() {
    if (m_client->connectToPlc()) {
        m_reconnIntervalMs = 500;
        m_reconnTimer->stop();
        m_pollTimer->start();
        emit connectionStateChanged(true);
    } else {
        m_reconnIntervalMs = qMin(m_reconnIntervalMs * 2, 10000);
        m_reconnTimer->setInterval(m_reconnIntervalMs);
    }
}
```

---

## 主窗口启动工作线程

```cpp
// mainwindow.cpp
void MainWindow::initWorker() {
    PlcConfig cfg;
    cfg.ip      = Config::instance()->get("plc/ip",   "192.168.1.1");
    cfg.port    = Config::instance()->get("plc/port",  502).toUInt();
    cfg.unitId  = Config::instance()->get("plc/unitId", 1).toUInt();

    m_workerThread = new QThread(this);
    m_worker = new PlcWorker(cfg);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, [this]{ m_worker->start(20); });
    connect(m_worker, &PlcWorker::motorDataUpdated,   m_motorPanel,  &MotorPanel::onDataUpdated);
    connect(m_worker, &PlcWorker::sensorDataUpdated,  m_sensorPanel, &SensorPanel::onDataUpdated);
    connect(m_worker, &PlcWorker::connectionStateChanged, this,      &MainWindow::onConnectionChanged);
    connect(m_worker, &PlcWorker::alarmTriggered,     m_alarmPanel,  &AlarmPanel::onAlarm);

    // UI → Worker 写指令（跨线程，用 Qt::QueuedConnection）
    connect(m_motorPanel, &MotorPanel::writeRegisterRequested,
            m_worker, &PlcWorker::writeRegister, Qt::QueuedConnection);

    m_workerThread->start();
}
```

---

## 常见 Modbus 错误处理

| 场景 | 原因 | 处理 |
|------|------|------|
| 连接超时 | PLC IP 错误 / 网线断开 | 自动重连，指数退避 |
| 响应功能码 = 请求码 + 0x80 | Modbus 异常响应 | 读取第9字节异常码，记录日志 |
| 异常码 0x01 | 功能码不支持 | 检查 AC702 Modbus 配置 |
| 异常码 0x02 | 寄存器地址不存在 | 检查 InProShop 映射范围 |
| 异常码 0x03 | 数据值超范围 | 检查写入值合法性 |
| 读取数据全为 0 | 映射未启用 | InProShop 中确认变量已映射到 MW 区 |
