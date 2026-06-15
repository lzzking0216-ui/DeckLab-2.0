#pragma once
#include <QMainWindow>
#include <QThread>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include "communication/PlcWorker.h"
#include "platform/PlatformWorker.h"
#include "motor/MotorPanel.h"
#include "platform/PlatformPanel.h"
#include "sensor/SensorPanel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    // Modbus TCP (AC702 PLC)
    void onPlcConnectClicked();
    void onPlcConnectionChanged(bool connected);

    // UDP (国辰正域 Stewart)
    void onUdpConnectClicked();
    void onUdpConnectionChanged(bool online);

    void onError(const QString& msg);

private:
    void setupUi();
    void applyNavyTheme();
    void initPlcWorker(const PlcConfig& cfg);
    void stopPlcWorker();
    void initPlatformWorker(const PlatformConfig& cfg);
    void stopPlatformWorker();

    // ---- Modbus TCP 连接配置区 ----
    QLineEdit*   m_plcIpEdit    = nullptr;
    QSpinBox*    m_plcPortSpin  = nullptr;
    QSpinBox*    m_unitIdSpin   = nullptr;
    QPushButton* m_plcConnBtn   = nullptr;
    QLabel*      m_plcStatusLbl = nullptr;

    // ---- UDP 连接配置区 ----
    QLineEdit*   m_udpIpEdit    = nullptr;
    QSpinBox*    m_udpCmdPort   = nullptr;
    QPushButton* m_udpConnBtn   = nullptr;
    QLabel*      m_udpStatusLbl = nullptr;

    // ---- 三大面板 ----
    MotorPanel*    m_motorPanel    = nullptr;  // 左：卷筒 + RSD
    PlatformPanel* m_platformPanel = nullptr;  // 中：动平台（UDP）
    SensorPanel*   m_sensorPanel   = nullptr;  // 右：拉线传感器

    // ---- Modbus Worker ----
    QThread*   m_plcThread = nullptr;
    PlcWorker* m_plcWorker = nullptr;

    // ---- UDP Platform Worker ----
    QThread*         m_platformThread = nullptr;
    PlatformWorker*  m_platformWorker = nullptr;

    bool m_plcConnected = false;
    bool m_udpOnline    = false;
};
