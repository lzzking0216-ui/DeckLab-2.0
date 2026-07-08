#include "PlatformWorker.h"
#include "UdpPlatformClient.h"

PlatformWorker::PlatformWorker(const PlatformConfig& cfg, QObject* parent)
    : QObject(parent)
    , m_cfg(cfg)
{}

void PlatformWorker::initialize()
{
    m_client = new UdpPlatformClient(m_cfg, this);

    connect(m_client, &UdpPlatformClient::platformStateReceived,
            this,     &PlatformWorker::onStateReceived);
    connect(m_client, &UdpPlatformClient::errorOccurred,
            this,     &PlatformWorker::errorOccurred);

    if (!m_client->bindSockets()) {
        emit connectionChanged(false);
        return;
    }

    // Socket 绑定成功只代表本机 UDP 端口可用；收到平台推送帧后才视为在线。
    m_online = false;
    m_pushReceived = false;
    m_missCount = 0;
    emit connectionChanged(false);

    // 看门狗：定期检查是否收到推送；若 kMaxMiss 次未收到则视为离线
    m_watchdog = new QTimer(this);
    m_watchdog->setInterval(200);
    connect(m_watchdog, &QTimer::timeout, this, &PlatformWorker::onWatchdogTimeout);
    m_watchdog->start();
}

void PlatformWorker::stop()
{
    if (m_watchdog) m_watchdog->stop();
    if (m_client)   m_client->close();
}

// -----------------------------------------------------------------------
// 设备控制
// -----------------------------------------------------------------------
void PlatformWorker::powerOn()
{
    if (m_client) m_client->writeValue(FuncCode::POWER, 1.0);
}

void PlatformWorker::powerOff()
{
    if (m_client) m_client->writeValue(FuncCode::POWER, 0.0);
}

void PlatformWorker::faultReset()
{
    if (m_client) m_client->writeValue(FuncCode::FAULT_RESET, 1.0);
}

// -----------------------------------------------------------------------
// 运动控制
// -----------------------------------------------------------------------
void PlatformWorker::setMotionMode(int mode)
{
    if (m_client) m_client->writeValue(FuncCode::MOTION_MODE, static_cast<double>(mode));
}

void PlatformWorker::sendPoseTarget(double x, double y, double z,
                                    double rx, double ry, double rz)
{
    if (!m_client) return;
    m_client->writeValues({
        {FuncCode::TARGET_X,  x},
        {FuncCode::TARGET_Y,  y},
        {FuncCode::TARGET_Z,  z},
        {FuncCode::TARGET_RX, rx},
        {FuncCode::TARGET_RY, ry},
        {FuncCode::TARGET_RZ, rz},
    });
}

void PlatformWorker::stopMotion()
{
    setMotionMode(static_cast<int>(MotionMode::Stop));
}

// -----------------------------------------------------------------------
// 内部槽
// -----------------------------------------------------------------------
void PlatformWorker::onStateReceived(const PlatformState& state)
{
    m_missCount    = 0;
    m_pushReceived = true;
    if (!m_online) {
        m_online = true;
        emit connectionChanged(true);
    }
    emit platformStateUpdated(state);
}

void PlatformWorker::onWatchdogTimeout()
{
    ++m_missCount;
    if (m_missCount >= kMaxMiss && m_online) {
        m_online = false;
        emit connectionChanged(false);
    }
}
