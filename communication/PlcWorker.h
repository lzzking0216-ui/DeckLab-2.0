#pragma once
#include <QObject>
#include <QTimer>
#include <QVector>
#include "ModbusTcpClient.h"
#include "RegisterMap.h"
#include "../motor/MotorController.h"
#include "../sensor/SensorData.h"

class PlcWorker : public QObject 
{
    Q_OBJECT
public:
    explicit PlcWorker(const PlcConfig& cfg, QObject* parent = nullptr);

    // 获取轴控制器（供 MotorPanel 连接信号）
    MotorController* controller(int axisId) const { return m_controllers.at(axisId); }

public slots:
    // 由 QThread::started 触发，在工作线程中初始化
    void initialize();
    void stop();

    // ---- 来自 UI 的轴控制命令 ----
    void enableAxis(int axisId);
    void disableAxis(int axisId);
    void faultResetAxis(int axisId);
    void setVelocity(int axisId, double rpm);
    void setPosition(int axisId, qint32 pulses, double approachRpm);
    void emergencyStopAll();

    // 来自 MotorController 信号的低层写接口（同线程直连）
    void writeRegister(quint16 addr, quint16 value);
    void writeRegisters(quint16 addr, QVector<quint16> values);

signals:
    void motorDataUpdated(int axisId, MotorData data);
    void sensorDataUpdated(SensorData data);
    void connectionStateChanged(bool connected);
    void errorOccurred(const QString& msg);

private slots:
    void pollCycle();
    void tryReconnect();

private:
    PlcConfig              m_cfg;
    ModbusTcpClient*       m_client      = nullptr;
    QTimer*                m_pollTimer   = nullptr;
    QTimer*                m_reconnTimer = nullptr;
    int                    m_reconnIntervalMs = 500;
    QVector<MotorController*> m_controllers;

    void readAllMotors();
    void readSensors();
    void writeCtrlWord(int axisId, quint16 cw);
    void clearLocalAxesCtrlWords();

    quint16 m_heartbeat = 0;
};
