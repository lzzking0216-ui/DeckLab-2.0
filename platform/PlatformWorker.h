#pragma once
#include <QObject>
#include <QTimer>
#include "PlatformData.h"

class UdpPlatformClient;

// -----------------------------------------------------------------------
// PlatformWorker — 国辰正域 Stewart 平台设备控制层
//
// 运行在独立 QThread 中，通过 UdpPlatformClient 与设备通讯。
// 接收 UI 层信号，发出实时状态信号。
// -----------------------------------------------------------------------
class PlatformWorker : public QObject {
    Q_OBJECT
public:
    explicit PlatformWorker(const PlatformConfig& cfg, QObject* parent = nullptr);

public slots:
    // 由 QThread::started 触发
    void initialize();
    void stop();

    // ---- 设备控制命令（来自 UI） ----
    void powerOn();
    void powerOff();
    void faultReset();

    // ---- 运动控制命令 ----
    void setMotionMode(int mode);
    void sendPoseTarget(double x, double y, double z,
                        double rx, double ry, double rz);
    void stopMotion();

signals:
    void platformStateUpdated(const PlatformState& state);
    void connectionChanged(bool online);
    void errorOccurred(const QString& msg);

private slots:
    void onStateReceived(const PlatformState& state);
    void onWatchdogTimeout();

private:
    PlatformConfig      m_cfg;
    UdpPlatformClient*  m_client      = nullptr;
    QTimer*             m_watchdog    = nullptr;   // 若超时未收到推送则报警

    bool    m_online       = false;
    int     m_missCount    = 0;
    bool    m_pushReceived = false;  // [调试/正式] 收到首帧推送后置 true，看门狗才开始计时
    static constexpr int kMaxMiss = 5;   // 5×200ms = 1s 无推送则离线
};
