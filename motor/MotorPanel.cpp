#include "MotorPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QMessageBox>

// 简约白色主题
static const char* kGroupStyle =
    "QGroupBox {"
    "  border: 1px solid #E5E7EB;"
    "  border-radius: 6px;"
    "  margin-top: 1.4em;"
    "  background-color: #F9FAFB;"
    "  color: #374151;"
    "  font-weight: bold;"
    "  font-size: 11pt;"
    "}"
    "QGroupBox::title {"
    "  subcontrol-origin: margin;"
    "  padding: 0 6px;"
    "}";

static const char* kLabelStyle =
    "color: #6B7280; font-size: 9pt;";
static const char* kValueStyle =
    "color: #111827; font-family: Consolas, monospace;"
    "font-size: 11pt; font-weight: bold;";
static const char* kStatePillNormal =
    "background: #F3F4F6; color: #6B7280;"
    "border-radius: 3px; padding: 2px 8px; font-size: 9pt;";
static const char* kStatePillEnabled =
    "background: #D1FAE5; color: #065F46;"
    "border-radius: 3px; padding: 2px 8px; font-size: 9pt; font-weight: bold;";
static const char* kStatePillFault =
    "background: #FEE2E2; color: #991B1B;"
    "border-radius: 3px; padding: 2px 8px; font-size: 9pt; font-weight: bold;";

static const char* kAxisNames[MotorPanel::LOCAL_AXIS_COUNT] = {
    "卷筒电机", "RSD 丝杆"
};

constexpr int MotorPanel::AXIS_IDS[];

MotorPanel::MotorPanel(QWidget* parent)
    : QWidget(parent)
{
    m_axisWidgets.resize(LOCAL_AXIS_COUNT);
    setupUi();
}

void MotorPanel::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(8);

    // 连接状态标签
    m_connLabel = new QLabel("● 未连接", this);
    m_connLabel->setStyleSheet("color: #9CA3AF; font-weight: bold;");
    mainLayout->addWidget(m_connLabel);

    // 构建两个轴的控制面板
    for (int i = 0; i < LOCAL_AXIS_COUNT; ++i) {
        buildAxisWidget(i, AXIS_IDS[i], QString::fromUtf8(kAxisNames[i]));
        mainLayout->addWidget(m_axisWidgets[i].group);
    }

    // 分隔线
    auto* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #E5E7EB;");
    mainLayout->addWidget(sep);

    // 急停按钮
    m_eStopBtn = new QPushButton("⚠  全局急停", this);
    m_eStopBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #DC2626; color: #FFFFFF;"
        "  font-size: 13pt; font-weight: bold;"
        "  border-radius: 6px; min-height: 46px;"
        "  border: 2px solid #B91C1C;"
        "}"
        "QPushButton:pressed { background-color: #B91C1C; }"
        "QPushButton:disabled {"
        "  background-color: #F3F4F6; color: #9CA3AF;"
        "  border-color: #E5E7EB;"
        "}");
    connect(m_eStopBtn, &QPushButton::clicked, this, [this] {
        if (QMessageBox::question(this, "急停确认",
                "确定要发出全局急停命令？\n所有轴将立即停止。",
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
            emit emergencyStopAllRequested();
    });
    mainLayout->addWidget(m_eStopBtn);
    mainLayout->addStretch();
}

void MotorPanel::buildAxisWidget(int index, int physicalId, const QString& name)
{
    AxisWidgets& w = m_axisWidgets[index];
    w.physicalId = physicalId;

    w.group = new QGroupBox(name, this);
    w.group->setStyleSheet(kGroupStyle);

    auto* vbox = new QVBoxLayout(w.group);
    vbox->setContentsMargins(10, 16, 10, 10);
    vbox->setSpacing(5);

    // 状态行
    auto* stateRow = new QHBoxLayout;
    auto* stateCaption = new QLabel("状态:", this);
    stateCaption->setStyleSheet(kLabelStyle);
    w.stateLabel = new QLabel("-- 未知 --", this);
    w.stateLabel->setStyleSheet(kStatePillNormal);
    w.stateLabel->setAlignment(Qt::AlignCenter);
    stateRow->addWidget(stateCaption);
    stateRow->addWidget(w.stateLabel, 1);
    stateRow->addStretch();
    vbox->addLayout(stateRow);

    // 实时数据
    auto* form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);
    form->setHorizontalSpacing(8);
    form->setVerticalSpacing(3);

    auto mkVal = [&]() -> QLabel* {
        auto* l = new QLabel("---", this);
        l->setStyleSheet(kValueStyle);
        return l;
    };
    auto mkCap = [&](const QString& t) -> QLabel* {
        auto* l = new QLabel(t, this);
        l->setStyleSheet(kLabelStyle);
        return l;
    };

    w.actVelLabel  = mkVal();
    w.actPosLabel  = mkVal();
    w.torqueLabel  = mkVal();
    w.errorLabel   = mkVal();

    form->addRow(mkCap("实际速度 (rpm):"),  w.actVelLabel);
    form->addRow(mkCap("实际位置 (pulse):"), w.actPosLabel);
    form->addRow(mkCap("力矩 (% 额定):"),   w.torqueLabel);
    form->addRow(mkCap("故障码:"),           w.errorLabel);
    vbox->addLayout(form);

    // 分隔线
    auto* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #E5E7EB;");
    vbox->addWidget(sep);

    // 使能控制行
    auto* ctrlRow = new QHBoxLayout;
    w.enableBtn = new QPushButton("使能", this);
    w.enableBtn->setStyleSheet(
        "QPushButton { background:#059669; color:white; border-radius:4px; padding:4px 14px; font-size:9pt; }"
        "QPushButton:pressed { background:#047857; }"
        "QPushButton:disabled { background:#F3F4F6; color:#9CA3AF; }");
    w.disableBtn = new QPushButton("去使能", this);
    w.disableBtn->setStyleSheet(
        "QPushButton { background:#EF4444; color:white; border-radius:4px; padding:4px 14px; font-size:9pt; }"
        "QPushButton:pressed { background:#DC2626; }"
        "QPushButton:disabled { background:#F3F4F6; color:#9CA3AF; }");
    w.faultResetBtn = new QPushButton("清故障", this);
    w.faultResetBtn->setStyleSheet(
        "QPushButton { background:#7C3AED; color:white; border-radius:4px; padding:4px 14px; font-size:9pt; }"
        "QPushButton:pressed { background:#6D28D9; }"
        "QPushButton:disabled { background:#F3F4F6; color:#9CA3AF; }");
    ctrlRow->addWidget(w.enableBtn);
    ctrlRow->addWidget(w.disableBtn);
    ctrlRow->addWidget(w.faultResetBtn);
    ctrlRow->addStretch();
    vbox->addLayout(ctrlRow);

    // 速度控制行
    auto* velRow = new QHBoxLayout;
    auto* velCap = new QLabel("目标速度:", this);
    velCap->setStyleSheet(kLabelStyle);
    w.velInput = new QDoubleSpinBox(this);
    w.velInput->setRange(-3000.0, 3000.0);
    w.velInput->setSingleStep(10.0);
    w.velInput->setSuffix(" rpm");
    w.velInput->setDecimals(1);
    w.velInput->setFixedWidth(120);
    w.velInput->setStyleSheet(
        "QDoubleSpinBox { background:#FFFFFF; color:#374151; border:1px solid #D1D5DB;"
        " border-radius:3px; padding:2px 4px; font-family:Consolas; }");
    w.velApplyBtn = new QPushButton("发送", this);
    w.velApplyBtn->setFixedWidth(56);
    w.velApplyBtn->setStyleSheet(
        "QPushButton { background:#2563EB; color:white; border-radius:4px; padding:3px 8px; font-size:9pt; }"
        "QPushButton:pressed { background:#1D4ED8; }"
        "QPushButton:disabled { background:#F3F4F6; color:#9CA3AF; }");
    velRow->addWidget(velCap);
    velRow->addWidget(w.velInput);
    velRow->addWidget(w.velApplyBtn);
    velRow->addStretch();
    vbox->addLayout(velRow);

    // 位置控制行
    auto* posRow = new QHBoxLayout;
    auto* posCap = new QLabel("目标位置:", this);
    posCap->setStyleSheet(kLabelStyle);
    w.posInput = new QSpinBox(this);
    w.posInput->setRange(INT_MIN, INT_MAX);
    w.posInput->setSuffix(" pulse");
    w.posInput->setFixedWidth(140);
    w.posInput->setStyleSheet(
        "QSpinBox { background:#FFFFFF; color:#374151; border:1px solid #D1D5DB;"
        " border-radius:3px; padding:2px 4px; font-family:Consolas; }");
    w.approachVelInput = new QDoubleSpinBox(this);
    w.approachVelInput->setRange(1.0, 3000.0);
    w.approachVelInput->setValue(100.0);
    w.approachVelInput->setSuffix(" rpm");
    w.approachVelInput->setDecimals(1);
    w.approachVelInput->setFixedWidth(100);
    w.approachVelInput->setStyleSheet(
        "QDoubleSpinBox { background:#FFFFFF; color:#374151; border:1px solid #D1D5DB;"
        " border-radius:3px; padding:2px 4px; font-family:Consolas; }");
    w.posApplyBtn = new QPushButton("定位", this);
    w.posApplyBtn->setFixedWidth(56);
    w.posApplyBtn->setStyleSheet(
        "QPushButton { background:#2563EB; color:white; border-radius:4px; padding:3px 8px; font-size:9pt; }"
        "QPushButton:pressed { background:#1D4ED8; }"
        "QPushButton:disabled { background:#F3F4F6; color:#9CA3AF; }");
    posRow->addWidget(posCap);
    posRow->addWidget(w.posInput);
    posRow->addWidget(new QLabel("@", this));
    posRow->addWidget(w.approachVelInput);
    posRow->addWidget(w.posApplyBtn);
    posRow->addStretch();
    vbox->addLayout(posRow);

    // 信号连接
    connect(w.enableBtn, &QPushButton::clicked, this, [this, index] {
        emit enableAxisRequested(m_axisWidgets[index].physicalId);
    });
    connect(w.disableBtn, &QPushButton::clicked, this, [this, index] {
        emit disableAxisRequested(m_axisWidgets[index].physicalId);
    });
    connect(w.faultResetBtn, &QPushButton::clicked, this, [this, index] {
        emit faultResetRequested(m_axisWidgets[index].physicalId);
    });
    connect(w.velApplyBtn, &QPushButton::clicked, this, [this, index] {
        emit setVelocityRequested(m_axisWidgets[index].physicalId,
                                  m_axisWidgets[index].velInput->value());
    });
    connect(w.posApplyBtn, &QPushButton::clicked, this, [this, index] {
        emit setPositionRequested(m_axisWidgets[index].physicalId,
                                  static_cast<qint32>(m_axisWidgets[index].posInput->value()),
                                  m_axisWidgets[index].approachVelInput->value());
    });

    // 初始禁用（未连接）
    w.enableBtn->setEnabled(false);
    w.disableBtn->setEnabled(false);
    w.faultResetBtn->setEnabled(false);
    w.velApplyBtn->setEnabled(false);
    w.posApplyBtn->setEnabled(false);
}

// -----------------------------------------------------------------------
// 数据更新槽：仅响应本地电机的轴 ID
// -----------------------------------------------------------------------
void MotorPanel::onMotorDataUpdated(int axisId, const MotorData& data)
{
    for (int i = 0; i < LOCAL_AXIS_COUNT; ++i) {
        if (m_axisWidgets[i].physicalId != axisId) continue;
        AxisWidgets& w = m_axisWidgets[i];

        w.actVelLabel->setText(QString::number(data.actualVel / 10.0, 'f', 1));
        w.actPosLabel->setText(QString::number(data.actualPos));
        w.torqueLabel->setText(QString::number(data.torque / 10.0, 'f', 1));

        if (data.errorCode != 0) {
            w.errorLabel->setText(
                QString("0x%1  %2").arg(data.errorCode, 4, 16, QChar('0'))
                                   .arg(faultCodeToString(data.errorCode)));
            w.errorLabel->setStyleSheet(
                "color:#DC2626; font-family:Consolas; font-size:9pt; font-weight:bold;");
        } else {
            w.errorLabel->setText("正常");
            w.errorLabel->setStyleSheet("color:#059669; font-family:Consolas; font-size:9pt;");
        }

        w.stateLabel->setText(data.stateName);
        if (data.faultActive)
            w.stateLabel->setStyleSheet(kStatePillFault);
        else if (data.enabled)
            w.stateLabel->setStyleSheet(kStatePillEnabled);
        else
            w.stateLabel->setStyleSheet(kStatePillNormal);

        // 速度/位置命令只在已使能且无故障时允许发送
        bool canRun = data.enabled && !data.faultActive;
        w.velApplyBtn->setEnabled(canRun);
        w.posApplyBtn->setEnabled(canRun);
        w.velApplyBtn->setToolTip(canRun ? "" : "请先使能电机");
        w.posApplyBtn->setToolTip(canRun ? "" : "请先使能电机");
        break;
    }
}

void MotorPanel::onConnectionChanged(bool connected)
{
    if (connected) {
        m_connLabel->setText("● 已连接");
        m_connLabel->setStyleSheet("color: #059669; font-weight: bold;");
    } else {
        m_connLabel->setText("● 未连接");
        m_connLabel->setStyleSheet("color: #9CA3AF; font-weight: bold;");
        for (auto& w : m_axisWidgets) {
            w.stateLabel->setText("-- 未知 --");
            w.stateLabel->setStyleSheet(kStatePillNormal);
            w.actVelLabel->setText("---");
            w.actPosLabel->setText("---");
            w.torqueLabel->setText("---");
            w.errorLabel->setText("---");
            w.errorLabel->setStyleSheet(kValueStyle);
        }
    }

    for (auto& w : m_axisWidgets) {
        w.enableBtn->setEnabled(connected);
        w.disableBtn->setEnabled(connected);
        w.faultResetBtn->setEnabled(connected);
        // 速度/位置按钮由 onMotorDataUpdated 根据使能状态控制，断开时禁用
        if (!connected) {
            //w.velApplyBtn->setEnabled(false);
            //w.posApplyBtn->setEnabled(false);
            w.velApplyBtn->setEnabled(connected);
            w.posApplyBtn->setEnabled(connected);
        }
    }
    m_eStopBtn->setEnabled(connected);
}
