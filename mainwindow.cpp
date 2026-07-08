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

    auto makeCabinetHeader = [this](const QString& title, const QString& color) {
        auto* header = new QLabel(title, this);
        header->setStyleSheet(QString(
            "color: %1; font-size: 13pt; font-weight: bold;"
            "padding: 8px 10px; background: #F9FAFB;"
            "border: 1px solid #E5E7EB; border-radius: 6px;").arg(color));
        return header;
    };

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(6);
    splitter->setStyleSheet(
        "QSplitter::handle { background: #E5E7EB; }"
        "QSplitter::handle:hover { background: #D1D5DB; }");

    // 快速回收单元（PLC / Modbus TCP）
    auto* cabinetB = new QWidget(splitter);
    cabinetB->setMinimumWidth(460);
    auto* cabinetBVBox = new QVBoxLayout(cabinetB);
    cabinetBVBox->setContentsMargins(0, 0, 0, 0);
    cabinetBVBox->setSpacing(6);
    cabinetBVBox->addWidget(makeCabinetHeader(QString::fromUtf8("快速回收单元"), "#2563EB"));

    auto* plcGroup = new QGroupBox(cabinetB);
    plcGroup->setStyleSheet(kConnGroupStyle);
    plcGroup->setFixedHeight(56);
    auto* plcRow = new QHBoxLayout(plcGroup);
    plcRow->setContentsMargins(10, 4, 10, 4);
    plcRow->setSpacing(8);

    auto* plcTag = new QLabel(QString::fromUtf8("PLC / Modbus TCP:"), plcGroup);
    plcTag->setStyleSheet("color:#2563EB; font-size:8pt; font-weight:bold;");
    plcRow->addWidget(plcTag);

    plcRow->addWidget(new QLabel("IP:", plcGroup));
    m_plcIpEdit = new QLineEdit("192.168.1.1", plcGroup);
    m_plcIpEdit->setMaximumWidth(130);
    m_plcIpEdit->setPlaceholderText("192.168.1.1");
    plcRow->addWidget(m_plcIpEdit);

    plcRow->addWidget(new QLabel(QString::fromUtf8("端口:"), plcGroup));
    m_plcPortSpin = new QSpinBox(plcGroup);
    m_plcPortSpin->setRange(1, 65535);
    m_plcPortSpin->setValue(502);
    m_plcPortSpin->setFixedWidth(68);
    plcRow->addWidget(m_plcPortSpin);

    plcRow->addWidget(new QLabel(QString::fromUtf8("从站:"), plcGroup));
    m_unitIdSpin = new QSpinBox(plcGroup);
    m_unitIdSpin->setRange(1, 247);
    m_unitIdSpin->setValue(1);
    m_unitIdSpin->setFixedWidth(52);
    plcRow->addWidget(m_unitIdSpin);

    m_plcConnBtn = new QPushButton(QString::fromUtf8("连接 PLC"), plcGroup);
    m_plcConnBtn->setFixedWidth(86);
    m_plcConnBtn->setStyleSheet(
        "QPushButton { background:#2563EB; color:white; border-radius:4px;"
        "  padding:4px 10px; font-size:9pt; font-weight:bold; }"
        "QPushButton:pressed { background:#1D4ED8; }"
        "QPushButton:disabled { background:#E5E7EB; color:#9CA3AF; }");
    connect(m_plcConnBtn, &QPushButton::clicked, this, &MainWindow::onPlcConnectClicked);
    plcRow->addWidget(m_plcConnBtn);

    m_plcStatusLbl = new QLabel(QString::fromUtf8("就绪"), plcGroup);
    m_plcStatusLbl->setStyleSheet("color: #6B7280; font-size: 8pt;");
    m_plcStatusLbl->setMinimumWidth(120);
    plcRow->addWidget(m_plcStatusLbl);
    plcRow->addStretch();
    cabinetBVBox->addWidget(plcGroup);

    auto* cabinetBStack = new QSplitter(Qt::Vertical, cabinetB);
    cabinetBStack->setHandleWidth(6);
    cabinetBStack->setStyleSheet(
        "QSplitter::handle { background: #E5E7EB; }"
        "QSplitter::handle:hover { background: #D1D5DB; }");
    m_motorPanel = new MotorPanel(cabinetBStack);
    m_sensorPanel = new SensorPanel(cabinetBStack);
    cabinetBStack->addWidget(m_motorPanel);
    cabinetBStack->addWidget(m_sensorPanel);
    cabinetBStack->setStretchFactor(0, 3);
    cabinetBStack->setStretchFactor(1, 2);
    cabinetBVBox->addWidget(cabinetBStack, 1);

    // 六自由度平台（UDP）
    auto* cabinetA = new QWidget(splitter);
    cabinetA->setMinimumWidth(620);
    auto* cabinetAVBox = new QVBoxLayout(cabinetA);
    cabinetAVBox->setContentsMargins(0, 0, 0, 0);
    cabinetAVBox->setSpacing(6);
    cabinetAVBox->addWidget(makeCabinetHeader(QString::fromUtf8("六自由度平台"), "#059669"));

    auto* udpGroup = new QGroupBox(cabinetA);
    udpGroup->setStyleSheet(kConnGroupStyle);
    udpGroup->setFixedHeight(56);
    auto* udpRow = new QHBoxLayout(udpGroup);
    udpRow->setContentsMargins(10, 4, 10, 4);
    udpRow->setSpacing(8);

    auto* udpTag = new QLabel(QString::fromUtf8("Stewart / UDP:"), udpGroup);
    udpTag->setStyleSheet("color:#059669; font-size:8pt; font-weight:bold;");
    udpRow->addWidget(udpTag);

    udpRow->addWidget(new QLabel("IP:", udpGroup));
    m_udpIpEdit = new QLineEdit("192.168.1.88", udpGroup);
    m_udpIpEdit->setMaximumWidth(130);
    m_udpIpEdit->setPlaceholderText("192.168.1.88");
    udpRow->addWidget(m_udpIpEdit);

    udpRow->addWidget(new QLabel(QString::fromUtf8("命令端口:"), udpGroup));
    m_udpCmdPort = new QSpinBox(udpGroup);
    m_udpCmdPort->setRange(1, 65535);
    m_udpCmdPort->setValue(2000);
    m_udpCmdPort->setFixedWidth(68);
    udpRow->addWidget(m_udpCmdPort);

    m_udpConnBtn = new QPushButton(QString::fromUtf8("连接平台"), udpGroup);
    m_udpConnBtn->setFixedWidth(86);
    m_udpConnBtn->setStyleSheet(
        "QPushButton { background:#059669; color:white; border-radius:4px;"
        "  padding:4px 10px; font-size:9pt; font-weight:bold; }"
        "QPushButton:pressed { background:#047857; }"
        "QPushButton:disabled { background:#E5E7EB; color:#9CA3AF; }");
    connect(m_udpConnBtn, &QPushButton::clicked, this, &MainWindow::onUdpConnectClicked);
    udpRow->addWidget(m_udpConnBtn);

    m_udpStatusLbl = new QLabel(QString::fromUtf8("就绪"), udpGroup);
    m_udpStatusLbl->setStyleSheet("color: #6B7280; font-size: 8pt;");
    m_udpStatusLbl->setMinimumWidth(120);
    udpRow->addWidget(m_udpStatusLbl);
    udpRow->addStretch();
    cabinetAVBox->addWidget(udpGroup);

    m_platformPanel = new PlatformPanel(cabinetA);
    cabinetAVBox->addWidget(m_platformPanel, 1);

    splitter->addWidget(cabinetB);
    splitter->addWidget(cabinetA);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);

    rootVBox->addWidget(splitter, 1);

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
