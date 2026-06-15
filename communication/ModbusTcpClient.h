#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QVector>

struct PlcConfig {
    QString ip               = "192.168.1.41";
    quint16 port             = 502;
    int     connectTimeoutMs = 2000;
    int     ioTimeoutMs      = 100;
    quint8  unitId           = 1;
};

class ModbusTcpClient : public QObject {
    Q_OBJECT
public:
    explicit ModbusTcpClient(const PlcConfig& cfg, QObject* parent = nullptr);
    ~ModbusTcpClient() override;

    bool connectToPlc();
    void disconnectFromPlc();
    bool isConnected() const;

    // FC 0x03 — 读保持寄存器
    bool readHoldingRegisters(quint16 startAddr, quint16 count, QVector<quint16>& out);
    // FC 0x06 — 写单个寄存器
    bool writeSingleRegister(quint16 addr, quint16 value);
    // FC 0x10 — 写多个寄存器
    bool writeMultipleRegisters(quint16 startAddr, const QVector<quint16>& values);

signals:
    void connectionStateChanged(bool connected);
    void errorOccurred(const QString& msg);

private:
    PlcConfig    m_cfg;
    QTcpSocket*  m_socket = nullptr;
    quint16      m_transactionId = 0;

    QByteArray buildReadRequest(quint16 startAddr, quint16 count);
    QByteArray buildWriteSingleRequest(quint16 addr, quint16 value);
    QByteArray buildWriteMultipleRequest(quint16 startAddr, const QVector<quint16>& values);

    bool sendAndReceive(quint16 expectedTid, const QByteArray& request,
                        QByteArray& response, int expectedLen);

    quint16 nextTransactionId() { return ++m_transactionId; }
};
