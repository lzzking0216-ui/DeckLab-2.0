#include "PlatformPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>

// -----------------------------------------------------------------------
// 简约白色主题
// -----------------------------------------------------------------------
static const char* kOuterGroup =
    "QGroupBox {"
    "  border: 1px solid #E5E7EB; border-radius: 6px;"
    "  background-color: #FFFFFF; color: #374151;"
    "  font-weight: bold; font-size: 12pt; margin-top: 1.2em;"
    "}"
    "QGroupBox::title { subcontrol-origin:margin; padding:0 8px; }";

static const char* kInnerGroup =
    "QGroupBox {"
    "  border: 1px solid #E5E7EB; border-radius: 5px;"
    "  background-color: #F9FAFB; color: #6B7280;"
    "  font-weight: bold; font-size: 9pt; margin-top: 1.2em;"
    "}"
    "QGroupBox::title { subcontrol-origin:margin; padding:0 6px; }";

static const char* kCapStyle   = "color:#6B7280; font-size:8pt;";
static const char* kValStyle   = "color:#111827; font-family:Consolas,monospace; font-size:10pt; font-weight:bold;";
static const char* kUnitStyle  = "color:#6B7280; font-size:8pt;";
static const char* kPillNorm   = "background:#F3F4F6;color:#6B7280;border-radius:3px;padding:2px 8px;font-size:8pt;";
static const char* kPillOn     = "background:#D1FAE5;color:#065F46;border-radius:3px;padding:2px 8px;font-size:8pt;font-weight:bold;";
static const char* kPillFault  = "background:#FEE2E2;color:#991B1B;border-radius:3px;padding:2px 8px;font-size:8pt;font-weight:bold;";
static const char* kPillMode   = "background:#DBEAFE;color:#1D4ED8;border-radius:3px;padding:2px 8px;font-size:8pt;";
static const char* kSpinStyle  =
    "background:#FFFFFF;color:#374151;border:1px solid #D1D5DB;"
    "border-radius:3px;padding:1px 3px;font-family:Consolas;font-size:9pt;";
static const char* kBtnGreen =
    "QPushButton{background:#059669;color:white;border-radius:3px;padding:3px 10px;font-size:8pt;}"
    "QPushButton:pressed{background:#047857;}"
    "QPushButton:disabled{background:#F3F4F6;color:#9CA3AF;}";
static const char* kBtnRed =
    "QPushButton{background:#EF4444;color:white;border-radius:3px;padding:3px 10px;font-size:8pt;}"
    "QPushButton:pressed{background:#DC2626;}"
    "QPushButton:disabled{background:#F3F4F6;color:#9CA3AF;}";
static const char* kBtnBlue =
    "QPushButton{background:#2563EB;color:white;border-radius:3px;padding:3px 10px;font-size:8pt;}"
    "QPushButton:pressed{background:#1D4ED8;}"
    "QPushButton:disabled{background:#F3F4F6;color:#9CA3AF;}";
static const char* kBtnReset =
    "QPushButton{background:#7C3AED;color:white;border-radius:3px;padding:3px 10px;font-size:8pt;}"
    "QPushButton:pressed{background:#6D28D9;}"
    "QPushButton:disabled{background:#F3F4F6;color:#9CA3AF;}";
static const char* kBtnStop =
    "QPushButton{background:#DC2626;color:white;border-radius:3px;padding:3px 10px;font-size:8pt;font-weight:bold;}"
    "QPushButton:pressed{background:#B91C1C;}"
    "QPushButton:disabled{background:#F3F4F6;color:#9CA3AF;}";

static const char* kPoseNames[6] = {"X", "Y", "Z", "RX", "RY", "RZ"};
static const char* kPoseUnits[6] = {"mm","mm","mm","°","°","°"};

// -----------------------------------------------------------------------
// 构造 & 布局
// -----------------------------------------------------------------------
PlatformPanel::PlatformPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void PlatformPanel::setupUi()
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(4, 4, 4, 4);
    outerLayout->setSpacing(0);

    auto* outerGroup = new QGroupBox("动平台  （国辰正域 Stewart  UDP 控制）", this);
    outerGroup->setStyleSheet(kOuterGroup);

    auto* vbox = new QVBoxLayout(outerGroup);
    vbox->setContentsMargins(8, 14, 8, 8);
    vbox->setSpacing(6);

    buildStatusSection(outerGroup, vbox);
    vbox->addWidget([&]() -> QFrame* {
        auto* f = new QFrame; f->setFrameShape(QFrame::HLine);
        f->setStyleSheet("color:#E5E7EB;"); return f;
    }());

    // 中间两列：左=当前位姿，右=目标位姿
    auto* midRow = new QHBoxLayout;
    midRow->setSpacing(6);

    auto* leftGroup = new QGroupBox("当前位姿（实时）", outerGroup);
    leftGroup->setStyleSheet(kInnerGroup);
    auto* leftVBox = new QVBoxLayout(leftGroup);
    leftVBox->setContentsMargins(8, 14, 8, 6);
    leftVBox->setSpacing(3);
    buildCurrentPoseSection(leftGroup, leftVBox);

    auto* rightGroup = new QGroupBox("目标位姿（姿态定点模式）", outerGroup);
    rightGroup->setStyleSheet(kInnerGroup);
    auto* rightVBox = new QVBoxLayout(rightGroup);
    rightVBox->setContentsMargins(8, 14, 8, 6);
    rightVBox->setSpacing(3);
    buildTargetPoseSection(rightGroup, rightVBox);

    midRow->addWidget(leftGroup, 1);
    midRow->addWidget(rightGroup, 1);
    vbox->addLayout(midRow);

    vbox->addWidget([&]() -> QFrame* {
        auto* f = new QFrame; f->setFrameShape(QFrame::HLine);
        f->setStyleSheet("color:#E5E7EB;"); return f;
    }());

    buildControlSection(outerGroup, vbox);

    vbox->addWidget([&]() -> QFrame* {
        auto* f = new QFrame; f->setFrameShape(QFrame::HLine);
        f->setStyleSheet("color:#E5E7EB;"); return f;
    }());

    buildChainSection(outerGroup, vbox);
    vbox->addStretch();
    outerLayout->addWidget(outerGroup, 1);

    updateButtonStates();
}

// -----------------------------------------------------------------------
// 状态区：连接状态 + 设备状态 + 电源控制
// -----------------------------------------------------------------------
void PlatformPanel::buildStatusSection(QWidget* parent, QVBoxLayout* vbox)
{
    auto* row = new QHBoxLayout;
    row->setSpacing(8);

    row->addWidget([&]() -> QLabel* {
        auto* l = new QLabel("UDP:", parent);
        l->setStyleSheet(kCapStyle); return l;
    }());
    m_connIndicator = new QLabel("离线", parent);
    m_connIndicator->setStyleSheet(kPillNorm);
    row->addWidget(m_connIndicator);

    row->addSpacing(8);

    row->addWidget([&]() -> QLabel* {
        auto* l = new QLabel("电源:", parent);
        l->setStyleSheet(kCapStyle); return l;
    }());
    m_powerState = new QLabel("--", parent);
    m_powerState->setStyleSheet(kPillNorm);
    row->addWidget(m_powerState);

    row->addSpacing(8);

    row->addWidget([&]() -> QLabel* {
        auto* l = new QLabel("模式:", parent);
        l->setStyleSheet(kCapStyle); return l;
    }());
    m_modeDisplay = new QLabel("--", parent);
    m_modeDisplay->setStyleSheet(kPillMode);
    row->addWidget(m_modeDisplay);

    row->addSpacing(8);

    m_faultIndicator = new QLabel("正常", parent);
    m_faultIndicator->setStyleSheet(kPillNorm);
    row->addWidget(m_faultIndicator);

    row->addStretch();

    m_powerOnBtn = new QPushButton("上电", parent);
    m_powerOnBtn->setStyleSheet(kBtnGreen);
    m_powerOnBtn->setFixedWidth(66);

    m_powerOffBtn = new QPushButton("下电", parent);
    m_powerOffBtn->setStyleSheet(kBtnRed);
    m_powerOffBtn->setFixedWidth(66);

    m_faultResetBtn = new QPushButton("清故障", parent);
    m_faultResetBtn->setStyleSheet(kBtnReset);
    m_faultResetBtn->setFixedWidth(72);

    row->addWidget(m_powerOnBtn);
    row->addWidget(m_powerOffBtn);
    row->addWidget(m_faultResetBtn);

    vbox->addLayout(row);

    connect(m_powerOnBtn,    &QPushButton::clicked, this, &PlatformPanel::powerOnRequested);
    connect(m_powerOffBtn,   &QPushButton::clicked, this, &PlatformPanel::powerOffRequested);
    connect(m_faultResetBtn, &QPushButton::clicked, this, &PlatformPanel::faultResetRequested);
}

// -----------------------------------------------------------------------
// 当前位姿显示（6 行）
// -----------------------------------------------------------------------
void PlatformPanel::buildCurrentPoseSection(QWidget* parent, QVBoxLayout* vbox)
{
    for (int i = 0; i < 6; ++i) {
        auto& d = m_poseDisplay[i];
        auto* row = new QHBoxLayout;
        row->setSpacing(4);

        d.label = new QLabel(QString::fromUtf8(kPoseNames[i]), parent);
        d.label->setStyleSheet("color:#374151; font-size:9pt; font-weight:bold;");
        d.label->setFixedWidth(28);

        d.value = new QLabel("---", parent);
        d.value->setStyleSheet(
            "color:#111827; font-family:Consolas,monospace; font-size:11pt; font-weight:bold;"
            "background:#FFFFFF; border:1px solid #E5E7EB; border-radius:3px; padding:1px 5px;");
        d.value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        d.value->setMinimumWidth(90);

        d.unit = new QLabel(QString::fromUtf8(kPoseUnits[i]), parent);
        d.unit->setStyleSheet(kUnitStyle);
        d.unit->setFixedWidth(24);

        row->addWidget(d.label);
        row->addWidget(d.value, 1);
        row->addWidget(d.unit);
        vbox->addLayout(row);
    }
}

// -----------------------------------------------------------------------
// 运动模式选择 + 急停
// -----------------------------------------------------------------------
void PlatformPanel::buildControlSection(QWidget* parent, QVBoxLayout* vbox)
{
    auto* row = new QHBoxLayout;
    row->setSpacing(8);

    row->addWidget([&]() -> QLabel* {
        auto* l = new QLabel("运动模式:", parent);
        l->setStyleSheet(kCapStyle); return l;
    }());

    m_modeCombo = new QComboBox(parent);
    m_modeCombo->setStyleSheet(
        "QComboBox { background:#FFFFFF; color:#374151; border:1px solid #D1D5DB;"
        "  border-radius:3px; padding:2px 6px; font-size:9pt; }"
        "QComboBox::drop-down { border:none; width:18px; }"
        "QComboBox QAbstractItemView { background:#FFFFFF; color:#374151;"
        "  selection-background-color:#2563EB; selection-color:#FFFFFF; }");
    m_modeCombo->setMinimumWidth(130);
    m_modeCombo->addItem("停止",       0);
    m_modeCombo->addItem("支链点动",   1);
    m_modeCombo->addItem("姿态点动",   2);
    m_modeCombo->addItem("姿态定点",   3);
    m_modeCombo->addItem("正弦运动",   4);
    m_modeCombo->addItem("增量运动",   5);
    m_modeCombo->addItem("跟随模式",   6);
    m_modeCombo->addItem("支链定点",  10);
    row->addWidget(m_modeCombo);

    m_modeApply = new QPushButton("设置", parent);
    m_modeApply->setStyleSheet(kBtnBlue);
    m_modeApply->setFixedWidth(58);
    row->addWidget(m_modeApply);

    row->addStretch();

    m_stopBtn = new QPushButton("急停", parent);
    m_stopBtn->setStyleSheet(kBtnStop);
    m_stopBtn->setFixedWidth(72);
    row->addWidget(m_stopBtn);

    vbox->addLayout(row);

    connect(m_modeApply, &QPushButton::clicked, this, [this] {
        emit setMotionModeRequested(m_modeCombo->currentData().toInt());
    });
    connect(m_stopBtn, &QPushButton::clicked, this, &PlatformPanel::stopMotionRequested);
}

// -----------------------------------------------------------------------
// 目标位姿输入（6 行 + 发送按钮）
// -----------------------------------------------------------------------
void PlatformPanel::buildTargetPoseSection(QWidget* parent, QVBoxLayout* vbox)
{
    static const double rangeMin[6] = {-500.0,-500.0,-500.0,-30.0,-30.0,-30.0};
    static const double rangeMax[6] = { 500.0, 500.0, 500.0, 30.0, 30.0, 30.0};
    static const double stepSize[6] = {  1.0,   1.0,   1.0,  0.1,  0.1,  0.1};
    static const int decimals[6]    = {  2,     2,     2,    3,    3,    3};

    for (int i = 0; i < 6; ++i) {
        auto& d = m_poseInput[i];
        auto* row = new QHBoxLayout;
        row->setSpacing(4);

        d.label = new QLabel(QString::fromUtf8(kPoseNames[i]), parent);
        d.label->setStyleSheet("color:#374151; font-size:9pt; font-weight:bold;");
        d.label->setFixedWidth(28);

        d.spin = new QDoubleSpinBox(parent);
        d.spin->setRange(rangeMin[i], rangeMax[i]);
        d.spin->setSingleStep(stepSize[i]);
        d.spin->setDecimals(decimals[i]);
        d.spin->setValue(0.0);
        d.spin->setStyleSheet(kSpinStyle);
        d.spin->setMinimumWidth(90);

        d.unit = new QLabel(QString::fromUtf8(kPoseUnits[i]), parent);
        d.unit->setStyleSheet(kUnitStyle);
        d.unit->setFixedWidth(24);

        row->addWidget(d.label);
        row->addWidget(d.spin, 1);
        row->addWidget(d.unit);
        vbox->addLayout(row);
    }

    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    m_sendTargetBtn = new QPushButton("发送目标位姿", parent);
    m_sendTargetBtn->setStyleSheet(kBtnBlue);
    m_sendTargetBtn->setFixedHeight(28);
    btnRow->addWidget(m_sendTargetBtn);
    vbox->addLayout(btnRow);

    connect(m_sendTargetBtn, &QPushButton::clicked, this, [this] {
        emit sendPoseTargetRequested(
            m_poseInput[0].spin->value(),
            m_poseInput[1].spin->value(),
            m_poseInput[2].spin->value(),
            m_poseInput[3].spin->value(),
            m_poseInput[4].spin->value(),
            m_poseInput[5].spin->value()
        );
    });
}

// -----------------------------------------------------------------------
// 支链长度显示（紧凑单行）
// -----------------------------------------------------------------------
void PlatformPanel::buildChainSection(QWidget* parent, QVBoxLayout* vbox)
{
    auto* row = new QHBoxLayout;
    row->setSpacing(12);

    row->addWidget([&]() -> QLabel* {
        auto* l = new QLabel("支链长度:", parent);
        l->setStyleSheet(kCapStyle); return l;
    }());

    for (int i = 0; i < 6; ++i) {
        auto* cap = new QLabel(QString("L%1:").arg(i + 1), parent);
        cap->setStyleSheet(kCapStyle);
        row->addWidget(cap);

        m_chainLabels[i] = new QLabel("---", parent);
        m_chainLabels[i]->setStyleSheet(
            "color:#374151; font-family:Consolas,monospace; font-size:9pt;");
        m_chainLabels[i]->setMinimumWidth(60);
        m_chainLabels[i]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(m_chainLabels[i]);

        auto* unit = new QLabel("mm", parent);
        unit->setStyleSheet(kUnitStyle);
        row->addWidget(unit);
    }
    row->addStretch();
    vbox->addLayout(row);
}

// -----------------------------------------------------------------------
// 模式名称
// -----------------------------------------------------------------------
QString PlatformPanel::modeName(int mode)
{
    switch (mode) {
    case 0:  return "停止";
    case 1:  return "支链点动";
    case 2:  return "姿态点动";
    case 3:  return "姿态定点";
    case 4:  return "正弦";
    case 5:  return "增量";
    case 6:  return "跟随";
    case 10: return "支链定点";
    default: return QString("模式%1").arg(mode);
    }
}

// -----------------------------------------------------------------------
// 按钮状态
// -----------------------------------------------------------------------
void PlatformPanel::updateButtonStates()
{
    m_powerOnBtn->setEnabled(m_online);
    m_powerOffBtn->setEnabled(m_online);
    m_faultResetBtn->setEnabled(m_online);
    m_modeApply->setEnabled(m_online);
    m_stopBtn->setEnabled(m_online);
    m_sendTargetBtn->setEnabled(m_online);
    m_modeCombo->setEnabled(m_online);
}

// -----------------------------------------------------------------------
// 槽
// -----------------------------------------------------------------------
void PlatformPanel::onConnectionChanged(bool online)
{
    m_online = online;
    m_connIndicator->setText(online ? "在线" : "离线");
    m_connIndicator->setStyleSheet(online ? kPillOn : kPillNorm);

    if (!online) {
        m_powerState->setText("--");
        m_powerState->setStyleSheet(kPillNorm);
        m_modeDisplay->setText("--");
        m_faultIndicator->setText("正常");
        m_faultIndicator->setStyleSheet(kPillNorm);

        for (auto& d : m_poseDisplay) d.value->setText("---");
        for (auto* lbl : m_chainLabels) if (lbl) lbl->setText("---");
    }
    updateButtonStates();
}

void PlatformPanel::onPlatformStateUpdated(const PlatformState& state)
{
    m_powerState->setText(state.powered ? "已上电" : "已下电");
    m_powerState->setStyleSheet(state.powered ? kPillOn : kPillNorm);

    m_faultIndicator->setText(state.faultActive ? "故障" : "正常");
    m_faultIndicator->setStyleSheet(state.faultActive ? kPillFault : kPillNorm);

    m_modeDisplay->setText(modeName(state.motionMode));

    const auto& p = state.currentPose;
    const double pvals[6] = {p.x, p.y, p.z, p.rx, p.ry, p.rz};
    const int decimals[6] = {2, 2, 2, 3, 3, 3};
    for (int i = 0; i < 6; ++i)
        m_poseDisplay[i].value->setText(QString::number(pvals[i], 'f', decimals[i]));

    for (int i = 0; i < 6; ++i)
        if (m_chainLabels[i])
            m_chainLabels[i]->setText(QString::number(state.chainLengths[i], 'f', 2));
}
