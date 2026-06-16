#include "mainwindow.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFrame>
#include <QStatusBar>
#include <QMessageBox>
#include <QSplitter>

// -----------------------------------------------------------------------
// 简约白色主题
// -----------------------------------------------------------------------
static const char* kWindowBg =
    "QMainWindow, QWidget#central { background-color: #FFFFFF; }"
    "QStatusBar { background: #F9FAFB; color: #6B7280; border-top: 1px solid #E5E7EB; }";

static const char* kConnGroupStyle =
    "QGroupBox {"
    "  border: 1px solid #E5E7EB;"
    "  border-radius: 6px;"
    "  background-color: #F9FAFB;"
    "  margin-top: 0px;"
    "}"
    "QLabel { color: #374151; }"
    "QLineEdit, QSpinBox {"
    "  background: #FFFFFF; color: #374151;"
    "  border: 1px solid #D1D5DB; border-radius: 3px;"
    "  padding: 2px 4px; font-family: Consolas;"
    "}";

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("DeckLab  —  运动控制系统");
    setMinimumSize(1400, 800);
    applyNavyTheme();
    setupUi();
}

MainWindow::~MainWindow()
{
    stopPlcWorker();
    stopPlatformWorker();
}

// -----------------------------------------------------------------------
// 全局简约白色主题
// -----------------------------------------------------------------------
void MainWindow::applyNavyTheme()
{
    qApp->setStyleSheet(
        "QMainWindow, QWidget { background-color: #FFFFFF; color: #374151; }"
        "QStatusBar { background: #F9FAFB; color: #6B7280;"
        "  border-top: 1px solid #E5E7EB; }"
        "QScrollBar:vertical { background: #F3F4F6; width: 8px; margin: 0; }"
        "QScrollBar::handle:vertical { background: #D1D5DB; border-radius: 4px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QFrame[frameShape=\"4\"] { color: #E5E7EB; }"
        "QSpinBox, QDoubleSpinBox, QLineEdit {"
        "  background: #FFFFFF; color: #374151;"
        "  border: 1px solid #D1D5DB; border-radius: 3px; padding: 2px 4px;"
        "}"
        "QSpinBox::up-button, QSpinBox::down-button,"
        "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {"
        "  background: #F3F4F6; border: none; width: 16px;"
        "}"
        "QComboBox { background:#FFFFFF; color:#374151; border:1px solid #D1D5DB;"
        "  border-radius:3px; padding:2px 4px; }"
        "QComboBox QAbstractItemView { background:#FFFFFF; color:#374151;"
        "  selection-background-color:#2563EB; selection-color:#FFFFFF; }"
        "QLabel { color: #374151; }"
        "QToolTip { background: #FFFFFF; color: #374151; border: 1px solid #D1D5DB; }"
        "QMessageBox { background: #FFFFFF; color: #374151; }"
        "QMessageBox QPushButton {"
        "  background: #2563EB; color: white; border-radius: 4px;"
        "  padding: 5px 18px; font-size: 9pt;"
        "}"
    );
}

// -----------------------------------------------------------------------
// UI 构建
// -----------------------------------------------------------------------
void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    central->setObjectName("central");
    setCentralWidget(central);

    auto* rootVBox = new QVBoxLayout(central);
    rootVBox->setContentsMargins(8, 8, 8, 8);
    rootVBox->setSpacing(6);

    // ====================================================================
    // 顶栏：两段连接配置（PLC Modbus  |  Stewart UDP）
    // ====================================================================
    auto* connGroup = new QGroupBox(this);
    connGroup->setStyleSheet(kConnGroupStyle);
    connGroup->setFixedHeight(52);

    auto* connRow = new QHBoxLayout(connGroup);
    connRow->setContentsMargins(10, 4, 10, 4);
    connRow->setSpacing(8);

    // —— Modbus TCP 段 ——
    auto* plcTag = new QLabel("汇川 AC702  Modbus TCP:", this);
    plcTag->setStyleSheet("color:#2563EB; font-size:8pt; font-weight:bold;");
    connRow->addWidget(plcTag);

    connRow->addWidget(new QLabel("IP:", this));
    m_plcIpEdit = new QLineEdit("192.168.1.1", this);
    m_plcIpEdit->setMaximumWidth(130);
    m_plcIpEdit->setPlaceholderText("192.168.1.1");
    connRow->addWidget(m_plcIpEdit);

    connRow->addWidget(new QLabel("端口:", this));
    m_plcPortSpin = new QSpinBox(this);
    m_plcPortSpin->setRange(1, 65535);
    m_plcPortSpin->setValue(502);
    m_plcPortSpin->setFixedWidth(68);
    connRow->addWidget(m_plcPortSpin);

    connRow->addWidget(new QLabel("从站:", this));
    m_unitIdSpin = new QSpinBox(this);
    m_unitIdSpin->setRange(1, 247);
    m_unitIdSpin->setValue(1);
    m_unitIdSpin->setFixedWidth(52);
    connRow->addWidget(m_unitIdSpin);

    m_plcConnBtn = new QPushButton("连接 PLC", this);
    m_plcConnBtn->setFixedWidth(86);
    m_plcConnBtn->setStyleSheet(
        "QPushButton { background:#2563EB; color:white; border-radius:4px;"
        "  padding:4px 10px; font-size:9pt; font-weight:bold; }"
        "QPushButton:pressed { background:#1D4ED8; }"
        "QPushButton:disabled { background:#E5E7EB; color:#9CA3AF; }");
    connect(m_plcConnBtn, &QPushButton::clicked, this, &MainWindow::onPlcConnectClicked);
    connRow->addWidget(m_plcConnBtn);

    m_plcStatusLbl = new QLabel("就绪", this);
    m_plcStatusLbl->setStyleSheet("color: #6B7280; font-size: 8pt;");
    m_plcStatusLbl->setMinimumWidth(140);
    connRow->addWidget(m_plcStatusLbl);

    // 分隔竖线
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet("color:#E5E7EB;");
    connRow->addWidget(sep);

    // —— UDP 段 ——
    auto* udpTag = new QLabel("国辰正域  Stewart  UDP:", this);
    udpTag->setStyleSheet("color:#059669; font-size:8pt; font-weight:bold;");
    connRow->addWidget(udpTag);

    connRow->addWidget(new QLabel("IP:", this));
    m_udpIpEdit = new QLineEdit("192.168.1.88", this);
    m_udpIpEdit->setMaximumWidth(130);
    m_udpIpEdit->setPlaceholderText("192.168.1.88");
    connRow->addWidget(m_udpIpEdit);

    connRow->addWidget(new QLabel("命令端口:", this));
    m_udpCmdPort = new QSpinBox(this);
    m_udpCmdPort->setRange(1, 65535);
    m_udpCmdPort->setValue(2000);
    m_udpCmdPort->setFixedWidth(68);
    connRow->addWidget(m_udpCmdPort);

    m_udpConnBtn = new QPushButton("连接平台", this);
    m_udpConnBtn->setFixedWidth(86);
    m_udpConnBtn->setStyleSheet(
        "QPushButton { background:#059669; color:white; border-radius:4px;"
        "  padding:4px 10px; font-size:9pt; font-weight:bold; }"
        "QPushButton:pressed { background:#047857; }"
        "QPushButton:disabled { background:#E5E7EB; color:#9CA3AF; }");
    connect(m_udpConnBtn, &QPushButton::clicked, this, &MainWindow::onUdpConnectClicked);
    connRow->addWidget(m_udpConnBtn);

    m_udpStatusLbl = new QLabel("就绪", this);
    m_udpStatusLbl->setStyleSheet("color: #6B7280; font-size: 8pt;");
    m_udpStatusLbl->setMinimumWidth(120);
    connRow->addWidget(m_udpStatusLbl);

    connRow->addStretch();
    rootVBox->addWidget(connGroup);

    // ====================================================================
    // 主区：三列布局（左 | 中 | 右）
    // ====================================================================
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(6);
    splitter->setStyleSheet(
        "QSplitter::handle { background: #E5E7EB; }"
        "QSplitter::handle:hover { background: #D1D5DB; }");

    // ── 左侧：本地电机控制（卷筒 + RSD）──────────────────────────
    auto* leftWidget = new QWidget(splitter);
    leftWidget->setMinimumWidth(260);
    auto* leftHeader = new QLabel("本地驱动控制  (Modbus TCP)", leftWidget);
    leftHeader->setStyleSheet(
        "color: #2563EB; font-size: 12pt; font-weight: bold;"
        "padding: 4px 8px; background: #F9FAFB;"
        "border-bottom: 1px solid #E5E7EB;");
    auto* leftVBox = new QVBoxLayout(leftWidget);
    leftVBox->setContentsMargins(0, 0, 0, 0);
    leftVBox->setSpacing(0);
    leftVBox->addWidget(leftHeader);
    m_motorPanel = new MotorPanel(leftWidget);
    leftVBox->addWidget(m_motorPanel, 1);

    // ── 中央：动平台 Stewart 控制（UDP）────────────────────────────
    auto* centerWidget = new QWidget(splitter);
    centerWidget->setMinimumWidth(620);
    auto* centerHeader = new QLabel("六自由度动平台控制  (UDP)", centerWidget);
    centerHeader->setStyleSheet(
        "color: #2563EB; font-size: 12pt; font-weight: bold;"
        "padding: 4px 8px; background: #F9FAFB;"
        "border-bottom: 1px solid #E5E7EB;");
    auto* centerVBox = new QVBoxLayout(centerWidget);
    centerVBox->setContentsMargins(0, 0, 0, 0);
    centerVBox->setSpacing(0);
    centerVBox->addWidget(centerHeader);
    m_platformPanel = new PlatformPanel(centerWidget);
    centerVBox->addWidget(m_platformPanel, 1);

    // ── 右侧：传感器数据 ────────────────────────────────────────
    auto* rightWidget = new QWidget(splitter);
    rightWidget->setMinimumWidth(220);
    auto* rightHeader = new QLabel("拉线传感器实时数据", rightWidget);
    rightHeader->setStyleSheet(
        "color: #2563EB; font-size: 12pt; font-weight: bold;"
        "padding: 4px 8px; background: #F9FAFB;"
        "border-bottom: 1px solid #E5E7EB;");
    auto* rightVBox = new QVBoxLayout(rightWidget);
    rightVBox->setContentsMargins(0, 0, 0, 0);
    rightVBox->setSpacing(0);
    rightVBox->addWidget(rightHeader);
    m_sensorPanel = new SensorPanel(rightWidget);
    rightVBox->addWidget(m_sensorPanel, 1);

    splitter->addWidget(leftWidget);
    splitter->addWidget(centerWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 5);
    splitter->setStretchFactor(2, 2);

    rootVBox->addWidget(splitter, 1);

    // 初始状态
    m_motorPanel->onConnectionChanged(false);
    m_platformPanel->onConnectionChanged(false);
    m_sensorPanel->onConnectionChanged(false);
}

// -----------------------------------------------------------------------
// Modbus TCP 连接
// -----------------------------------------------------------------------
void MainWindow::onPlcConnectClicked()
{
    if (m_plcThread) {
        stopPlcWorker();
        m_plcConnBtn->setText("连接 PLC");
        m_plcIpEdit->setEnabled(true);
        m_plcPortSpin->setEnabled(true);
        m_unitIdSpin->setEnabled(true);
        m_plcStatusLbl->setText("已断开");
        m_plcStatusLbl->setStyleSheet("color: #6B7280; font-size: 8pt;");
        m_motorPanel->onConnectionChanged(false);
        m_sensorPanel->onConnectionChanged(false);
        m_plcConnected = false;
    } else {
        PlcConfig cfg;
        cfg.ip     = m_plcIpEdit->text().trimmed();
        cfg.port   = static_cast<quint16>(m_plcPortSpin->value());
        cfg.unitId = static_cast<quint8>(m_unitIdSpin->value());
        if (cfg.ip.isEmpty()) {
            QMessageBox::warning(this, "配置错误", "请输入 PLC IP 地址");
            return;
        }
        m_plcConnBtn->setEnabled(false);
        m_plcStatusLbl->setText(QString("正在连接 %1:%2 …").arg(cfg.ip).arg(cfg.port));
        m_plcStatusLbl->setStyleSheet("color: #D97706; font-size: 8pt;");
        initPlcWorker(cfg);
    }
}

void MainWindow::initPlcWorker(const PlcConfig& cfg)
{
    m_plcThread = new QThread(this);
    m_plcWorker = new PlcWorker(cfg);
    m_plcWorker->moveToThread(m_plcThread);

    connect(m_plcThread, &QThread::started,  m_plcWorker, &PlcWorker::initialize);
    connect(m_plcThread, &QThread::finished, m_plcWorker, &QObject::deleteLater);

    connect(m_plcWorker, &PlcWorker::connectionStateChanged,
            this, &MainWindow::onPlcConnectionChanged, Qt::QueuedConnection);
    connect(m_plcWorker, &PlcWorker::motorDataUpdated,
            m_motorPanel, &MotorPanel::onMotorDataUpdated, Qt::QueuedConnection);
    connect(m_plcWorker, &PlcWorker::sensorDataUpdated,
            m_sensorPanel, &SensorPanel::onSensorDataUpdated, Qt::QueuedConnection);
    connect(m_plcWorker, &PlcWorker::errorOccurred,
            this, &MainWindow::onError, Qt::QueuedConnection);

    // 本地电机 UI → Worker
    connect(m_motorPanel, &MotorPanel::enableAxisRequested,
            m_plcWorker, &PlcWorker::enableAxis, Qt::QueuedConnection);
    connect(m_motorPanel, &MotorPanel::disableAxisRequested,
            m_plcWorker, &PlcWorker::disableAxis, Qt::QueuedConnection);
    connect(m_motorPanel, &MotorPanel::faultResetRequested,
            m_plcWorker, &PlcWorker::faultResetAxis, Qt::QueuedConnection);
    connect(m_motorPanel, &MotorPanel::setVelocityRequested,
            m_plcWorker, &PlcWorker::setVelocity, Qt::QueuedConnection);
    connect(m_motorPanel, &MotorPanel::setPositionRequested,
            m_plcWorker, &PlcWorker::setPosition, Qt::QueuedConnection);
    connect(m_motorPanel, &MotorPanel::emergencyStopAllRequested,
            m_plcWorker, &PlcWorker::emergencyStopAll, Qt::QueuedConnection);

    m_plcThread->start();

    m_plcIpEdit->setEnabled(false);
    m_plcPortSpin->setEnabled(false);
    m_unitIdSpin->setEnabled(false);
    m_plcConnBtn->setText("断开 PLC");
    m_plcConnBtn->setEnabled(true);
}

void MainWindow::stopPlcWorker()
{
    if (!m_plcThread) return;
    QMetaObject::invokeMethod(m_plcWorker, &PlcWorker::stop, Qt::BlockingQueuedConnection);
    m_plcThread->quit();
    m_plcThread->wait(3000);
    m_plcThread = nullptr;
    m_plcWorker = nullptr;
}

void MainWindow::onPlcConnectionChanged(bool connected)
{
    m_plcConnected = connected;
    m_motorPanel->onConnectionChanged(connected);
    m_sensorPanel->onConnectionChanged(connected);

    if (connected) {
        m_plcStatusLbl->setText(
            QString("已连接  %1:%2").arg(m_plcIpEdit->text()).arg(m_plcPortSpin->value()));
        m_plcStatusLbl->setStyleSheet("color: #059669; font-size: 8pt; font-weight: bold;");
    } else {
        m_plcStatusLbl->setText("连接断开，重连中…");
        m_plcStatusLbl->setStyleSheet("color: #D97706; font-size: 8pt;");
    }
}

// -----------------------------------------------------------------------
// UDP 平台连接
// -----------------------------------------------------------------------
void MainWindow::onUdpConnectClicked()
{
    if (m_platformThread) {
        stopPlatformWorker();
        m_udpConnBtn->setText("连接平台");
        m_udpIpEdit->setEnabled(true);
        m_udpCmdPort->setEnabled(true);
        m_udpStatusLbl->setText("已断开");
        m_udpStatusLbl->setStyleSheet("color: #6B7280; font-size: 8pt;");
        m_platformPanel->onConnectionChanged(false);
        m_udpOnline = false;
    } else {
        PlatformConfig cfg;
        cfg.ip      = m_udpIpEdit->text().trimmed();
        cfg.cmdPort = static_cast<quint16>(m_udpCmdPort->value());
        if (cfg.ip.isEmpty()) {
            QMessageBox::warning(this, "配置错误", "请输入 Stewart 控制器 IP 地址");
            return;
        }
        m_udpConnBtn->setEnabled(false);
        m_udpStatusLbl->setText(QString("正在连接 %1:%2 …").arg(cfg.ip).arg(cfg.cmdPort));
        m_udpStatusLbl->setStyleSheet("color: #D97706; font-size: 8pt;");
        initPlatformWorker(cfg);
    }
}

void MainWindow::initPlatformWorker(const PlatformConfig& cfg)
{
    m_platformThread = new QThread(this);
    m_platformWorker = new PlatformWorker(cfg);
    m_platformWorker->moveToThread(m_platformThread);

    connect(m_platformThread, &QThread::started,
            m_platformWorker, &PlatformWorker::initialize);
    connect(m_platformThread, &QThread::finished,
            m_platformWorker, &QObject::deleteLater);

    connect(m_platformWorker, &PlatformWorker::connectionChanged,
            this, &MainWindow::onUdpConnectionChanged, Qt::QueuedConnection);
    connect(m_platformWorker, &PlatformWorker::platformStateUpdated,
            m_platformPanel, &PlatformPanel::onPlatformStateUpdated, Qt::QueuedConnection);
    connect(m_platformWorker, &PlatformWorker::errorOccurred,
            this, &MainWindow::onError, Qt::QueuedConnection);

    // 动平台 UI → Worker
    connect(m_platformPanel, &PlatformPanel::powerOnRequested,
            m_platformWorker, &PlatformWorker::powerOn, Qt::QueuedConnection);
    connect(m_platformPanel, &PlatformPanel::powerOffRequested,
            m_platformWorker, &PlatformWorker::powerOff, Qt::QueuedConnection);
    connect(m_platformPanel, &PlatformPanel::faultResetRequested,
            m_platformWorker, &PlatformWorker::faultReset, Qt::QueuedConnection);
    connect(m_platformPanel, &PlatformPanel::setMotionModeRequested,
            m_platformWorker, &PlatformWorker::setMotionMode, Qt::QueuedConnection);
    connect(m_platformPanel, &PlatformPanel::sendPoseTargetRequested,
            m_platformWorker, &PlatformWorker::sendPoseTarget, Qt::QueuedConnection);
    connect(m_platformPanel, &PlatformPanel::stopMotionRequested,
            m_platformWorker, &PlatformWorker::stopMotion, Qt::QueuedConnection);

    m_platformThread->start();

    m_udpIpEdit->setEnabled(false);
    m_udpCmdPort->setEnabled(false);
    m_udpConnBtn->setText("断开平台");
    m_udpConnBtn->setEnabled(true);
}

void MainWindow::stopPlatformWorker()
{
    if (!m_platformThread) return;
    QMetaObject::invokeMethod(m_platformWorker, &PlatformWorker::stop,
                              Qt::BlockingQueuedConnection);
    m_platformThread->quit();
    m_platformThread->wait(3000);
    m_platformThread = nullptr;
    m_platformWorker = nullptr;
}

void MainWindow::onUdpConnectionChanged(bool online)
{
    m_udpOnline = online;
    m_platformPanel->onConnectionChanged(online);

    if (online) {
        m_udpStatusLbl->setText(
            QString("在线  %1:%2").arg(m_udpIpEdit->text()).arg(m_udpCmdPort->value()));
        m_udpStatusLbl->setStyleSheet("color: #059669; font-size: 8pt; font-weight: bold;");
    } else {
        m_udpStatusLbl->setText("无推送数据…");
        m_udpStatusLbl->setStyleSheet("color: #D97706; font-size: 8pt;");
    }
}

void MainWindow::onError(const QString& msg)
{
    statusBar()->showMessage(msg, 5000);
}
