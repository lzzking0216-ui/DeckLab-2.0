#include "PlatformPanel.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QPixmap>
#include <QRadioButton>
#include <QSlider>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtMath>

namespace {

constexpr const char* kAxisNames[6] = {"Tx", "Ty", "Tz", "Rx", "Ry", "Rz"};
constexpr const char* kAxisUnits[6] = {"mm", "mm", "mm", "deg", "deg", "deg"};
constexpr double kTargetMin[6] = {-500.0, -500.0, -500.0, -30.0, -30.0, -30.0};
constexpr double kTargetMax[6] = { 500.0,  500.0,  500.0,  30.0,  30.0,  30.0};
constexpr int kTargetDecimals[6] = {5, 5, 5, 5, 5, 5};

const char* kPanel =
    "QWidget#platformCard{background:#FFFFFF;border:1px solid #E5E7EB;border-radius:6px;}";
const char* kHeader =
    "background:#F8FAFC;color:#0F172A;font-size:11pt;font-weight:bold;"
    "border-bottom:1px solid #E5E7EB;padding:8px 10px;";
const char* kCaption = "color:#64748B;font-size:9pt;";
const char* kValue = "color:#0F172A;font-family:Consolas,monospace;font-size:10pt;font-weight:bold;";
const char* kMuted = "color:#94A3B8;font-size:8pt;";
const char* kButton =
    "QPushButton{background:#FFFFFF;color:#1F2937;border:1px solid #CBD5E1;"
    "border-radius:4px;padding:5px 12px;font-size:9pt;}"
    "QPushButton:hover{background:#F8FAFC;border-color:#94A3B8;}"
    "QPushButton:pressed{background:#E2E8F0;}"
    "QPushButton:disabled{background:#F8FAFC;color:#B6BBC4;border-color:#E5E7EB;}";
const char* kPrimaryButton =
    "QPushButton{background:#059669;color:white;border:1px solid #047857;"
    "border-radius:4px;padding:6px 14px;font-size:9pt;font-weight:bold;}"
    "QPushButton:hover{background:#047857;}"
    "QPushButton:pressed{background:#065F46;}"
    "QPushButton:disabled{background:#E5E7EB;color:#94A3B8;border-color:#E5E7EB;}";
const char* kDangerButton =
    "QPushButton{background:#FFFFFF;color:#B91C1C;border:1px solid #FCA5A5;"
    "border-radius:4px;padding:5px 12px;font-size:9pt;}"
    "QPushButton:hover{background:#FEF2F2;}"
    "QPushButton:pressed{background:#FEE2E2;}"
    "QPushButton:disabled{background:#F8FAFC;color:#B6BBC4;border-color:#E5E7EB;}";
const char* kSpin =
    "QDoubleSpinBox{background:#FFFFFF;color:#0F172A;border:1px solid #CBD5E1;"
    "border-radius:4px;padding:3px 5px;font-family:Consolas;font-size:9pt;}"
    "QDoubleSpinBox:disabled{background:#F8FAFC;color:#94A3B8;}";
const char* kCombo =
    "QComboBox{background:#FFFFFF;color:#0F172A;border:1px solid #CBD5E1;"
    "border-radius:4px;padding:4px 8px;font-size:9pt;}"
    "QComboBox:disabled{background:#F8FAFC;color:#94A3B8;}";

QLabel* makeSectionTitle(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setStyleSheet("color:#0F172A;font-size:10pt;font-weight:bold;padding-top:4px;");
    return label;
}

QLabel* makeHint(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setStyleSheet("background:#F8FAFC;color:#64748B;border:1px solid #E5E7EB;"
                         "border-radius:5px;padding:8px;font-size:8pt;");
    return label;
}

class PlatformImage : public QWidget {
public:
    explicit PlatformImage(QWidget* parent = nullptr) : QWidget(parent)
    {
        setMinimumSize(420, 420);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        loadImage();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.fillRect(rect(), QColor("#FFFFFF"));

        const QRectF stage = rect().adjusted(18, 18, -18, -18);
        p.setPen(QPen(QColor("#E5E7EB"), 1));
        p.setBrush(QColor("#F8FAFC"));
        p.drawRoundedRect(stage, 8, 8);

        if (!m_image.isNull()) {
            const QPixmap scaled = m_image.scaled(stage.size().toSize(), Qt::KeepAspectRatio,
                                                  Qt::SmoothTransformation);
            const QPointF topLeft(stage.center().x() - scaled.width() / 2.0,
                                  stage.center().y() - scaled.height() / 2.0);
            p.drawPixmap(topLeft, scaled);
        } else {
            p.setPen(QColor("#64748B"));
            p.drawText(stage, Qt::AlignCenter,
                       QString::fromUtf8("请将平台图片保存为\nassets/platform/stewart_platform.png"));
        }
    }

private:
    void loadImage()
    {
        const QString relativePath = QStringLiteral("assets/platform/stewart_platform.png");
        const QStringList candidates = {
            QDir::current().absoluteFilePath(relativePath),
            QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(relativePath),
            QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../../") + relativePath),
        };

        for (const QString& path : candidates) {
            if (QFileInfo::exists(path) && m_image.load(path)) {
                return;
            }
        }
    }

    QPixmap m_image;
};

} // namespace

PlatformPanel::PlatformPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void PlatformPanel::setupUi()
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->setSpacing(8);

    root->addWidget(buildSystemPane(this), 3);
    root->addWidget(buildMotionPane(this), 2);

    updateTargetRanges();
    updatePoseReadouts();
    updateButtonStates();
}

QWidget* PlatformPanel::buildHeader(const QString& title, QWidget* parent)
{
    auto* header = new QWidget(parent);
    header->setStyleSheet(kHeader);
    auto* row = new QHBoxLayout(header);
    row->setContentsMargins(10, 4, 10, 4);
    row->setSpacing(8);
    auto* label = new QLabel(title, header);
    label->setStyleSheet("background:transparent;");
    row->addWidget(label);
    row->addStretch();
    return header;
}

QWidget* PlatformPanel::buildSystemPane(QWidget* parent)
{
    auto* pane = new QWidget(parent);
    pane->setObjectName("platformCard");
    pane->setStyleSheet(kPanel);
    auto* box = new QVBoxLayout(pane);
    box->setContentsMargins(0, 0, 0, 0);
    box->setSpacing(0);
    box->addWidget(buildHeader(QString::fromUtf8("系统主显示"), pane));

    auto* body = new QWidget(pane);
    auto* bodyRow = new QHBoxLayout(body);
    bodyRow->setContentsMargins(14, 14, 14, 14);
    bodyRow->setSpacing(14);
    bodyRow->addWidget(new PlatformImage(body), 1);

    auto* readout = new QWidget(body);
    readout->setMinimumWidth(190);
    readout->setMaximumWidth(240);
    auto* readoutBox = new QVBoxLayout(readout);
    readoutBox->setContentsMargins(0, 0, 0, 0);
    readoutBox->setSpacing(8);
    readoutBox->addWidget(makeSectionTitle(QString::fromUtf8("实时姿态"), readout));

    for (int i = 0; i < 6; ++i) {
        auto* line = new QWidget(readout);
        line->setStyleSheet("background:#F8FAFC;border:1px solid #E5E7EB;border-radius:5px;");
        auto* row = new QHBoxLayout(line);
        row->setContentsMargins(8, 5, 8, 5);
        row->setSpacing(6);
        m_poseRows[i].caption = new QLabel(kAxisNames[i], line);
        m_poseRows[i].caption->setStyleSheet(kCaption);
        m_poseRows[i].value = new QLabel("0.00000", line);
        m_poseRows[i].value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_poseRows[i].value->setStyleSheet(kValue);
        m_poseRows[i].unit = new QLabel(kAxisUnits[i], line);
        m_poseRows[i].unit->setStyleSheet(kCaption);
        row->addWidget(m_poseRows[i].caption);
        row->addWidget(m_poseRows[i].value, 1);
        row->addWidget(m_poseRows[i].unit);
        readoutBox->addWidget(line);
    }
    readoutBox->addStretch();
    bodyRow->addWidget(readout);

    box->addWidget(body, 1);
    return pane;
}

QWidget* PlatformPanel::buildMotionPane(QWidget* parent)
{
    auto* pane = new QWidget(parent);
    pane->setObjectName("platformCard");
    pane->setStyleSheet(kPanel);
    auto* box = new QVBoxLayout(pane);
    box->setContentsMargins(0, 0, 0, 0);
    box->setSpacing(0);
    box->addWidget(buildHeader(QString::fromUtf8("运动控制"), pane));

    auto* status = new QWidget(pane);
    auto* statusRow = new QHBoxLayout(status);
    statusRow->setContentsMargins(12, 10, 12, 4);
    statusRow->setSpacing(6);
    m_connIndicator = new QLabel(QString::fromUtf8("离线"), status);
    m_powerState = new QLabel("--", status);
    m_modeDisplay = new QLabel("--", status);
    m_faultIndicator = new QLabel(QString::fromUtf8("正常"), status);
    for (auto* lbl : {m_connIndicator, m_powerState, m_modeDisplay, m_faultIndicator}) {
        lbl->setStyleSheet("background:#F1F5F9;color:#64748B;border-radius:4px;padding:3px 8px;font-size:8pt;");
    }
    statusRow->addWidget(m_connIndicator);
    statusRow->addWidget(m_powerState);
    statusRow->addWidget(m_modeDisplay, 1);
    statusRow->addWidget(m_faultIndicator);
    box->addWidget(status);

    auto* powerRowWidget = new QWidget(pane);
    auto* powerRow = new QHBoxLayout(powerRowWidget);
    powerRow->setContentsMargins(12, 4, 12, 8);
    powerRow->setSpacing(8);
    m_powerOnBtn = new QPushButton(QString::fromUtf8("上电"), powerRowWidget);
    m_powerOffBtn = new QPushButton(QString::fromUtf8("下电"), powerRowWidget);
    m_faultResetBtn = new QPushButton(QString::fromUtf8("清故障"), powerRowWidget);
    m_powerOnBtn->setStyleSheet(kPrimaryButton);
    m_powerOffBtn->setStyleSheet(kButton);
    m_faultResetBtn->setStyleSheet(kDangerButton);
    powerRow->addWidget(m_powerOnBtn);
    powerRow->addWidget(m_powerOffBtn);
    powerRow->addWidget(m_faultResetBtn);
    box->addWidget(powerRowWidget);
    connect(m_powerOnBtn, &QPushButton::clicked, this, &PlatformPanel::powerOnRequested);
    connect(m_powerOffBtn, &QPushButton::clicked, this, &PlatformPanel::powerOffRequested);
    connect(m_faultResetBtn, &QPushButton::clicked, this, &PlatformPanel::faultResetRequested);

    m_motionStack = new QStackedWidget(pane);
    m_motionStack->addWidget(buildFixedPointPage(m_motionStack));
    m_motionStack->addWidget(buildIncrementPage(m_motionStack));
    m_motionStack->addWidget(buildSinePage(m_motionStack));
    m_motionStack->addWidget(buildTrajectoryPage(m_motionStack));
    box->addWidget(m_motionStack, 1);

    auto* modeRowWidget = new QWidget(pane);
    auto* modeRow = new QHBoxLayout(modeRowWidget);
    modeRow->setContentsMargins(12, 8, 12, 10);
    modeRow->setSpacing(6);
    m_modeButtons = new QButtonGroup(this);
    m_modeButtons->setExclusive(true);
    addModeButton(QString::fromUtf8("定点"), 0, modeRow);
    addModeButton(QString::fromUtf8("增量"), 1, modeRow);
    addModeButton(QString::fromUtf8("正弦"), 2, modeRow);
    addModeButton(QString::fromUtf8("轨迹"), 3, modeRow);
    box->addWidget(modeRowWidget);
    selectMotionPage(0);
    connect(m_modeButtons, &QButtonGroup::idClicked, this, &PlatformPanel::selectMotionPage);

    return pane;
}

void PlatformPanel::addModeButton(const QString& text, int pageIndex, QHBoxLayout* row)
{
    auto* btn = new QToolButton(this);
    btn->setText(text);
    btn->setCheckable(true);
    btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btn->setStyleSheet(
        "QToolButton{background:#F8FAFC;color:#475569;border:1px solid #E5E7EB;"
        "border-radius:5px;padding:6px 10px;font-size:9pt;}"
        "QToolButton:checked{background:#ECFDF5;color:#047857;border-color:#34D399;font-weight:bold;}"
        "QToolButton:disabled{color:#CBD5E1;}");
    m_modeButtons->addButton(btn, pageIndex);
    row->addWidget(btn);
}

QWidget* PlatformPanel::buildFixedPointPage(QWidget* parent)
{
    auto* page = new QWidget(parent);
    auto* box = new QVBoxLayout(page);
    box->setContentsMargins(14, 8, 14, 8);
    box->setSpacing(8);
    box->addWidget(makeSectionTitle(QString::fromUtf8("定点运动"), page));

    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(6);
    for (int i = 0; i < 6; ++i) {
        m_targetRows[i].caption = new QLabel(QString("%1 (%2)").arg(kAxisNames[i], kAxisUnits[i]), page);
        m_targetRows[i].caption->setStyleSheet(kCaption);
        m_targetRows[i].spin = new QDoubleSpinBox(page);
        m_targetRows[i].spin->setStyleSheet(kSpin);
        m_targetRows[i].spin->setDecimals(kTargetDecimals[i]);
        m_targetRows[i].spin->setAlignment(Qt::AlignRight);
        m_targetRows[i].spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_targetRows[i].unit = new QLabel(kAxisUnits[i], page);
        m_targetRows[i].unit->setStyleSheet(kCaption);
        grid->addWidget(m_targetRows[i].caption, i, 0);
        grid->addWidget(m_targetRows[i].spin, i, 1);
        grid->addWidget(m_targetRows[i].unit, i, 2);
    }
    box->addLayout(grid);

    auto* coordRow = new QHBoxLayout;
    coordRow->addWidget(new QLabel(QString::fromUtf8("坐标系"), page));
    m_coordCombo = new QComboBox(page);
    m_coordCombo->setStyleSheet(kCombo);
    m_coordCombo->addItem(QString::fromUtf8("世界坐标系"));
    m_coordCombo->addItem(QString::fromUtf8("平台坐标系"));
    coordRow->addWidget(m_coordCombo, 1);
    box->addLayout(coordRow);

    auto* btnRow = new QHBoxLayout;
    m_fixedExecuteBtn = new QPushButton(QString::fromUtf8("执行"), page);
    m_fixedResetBtn = new QPushButton(QString::fromUtf8("重置输入"), page);
    m_fixedExecuteBtn->setStyleSheet(kPrimaryButton);
    m_fixedResetBtn->setStyleSheet(kButton);
    btnRow->addWidget(m_fixedExecuteBtn);
    btnRow->addWidget(m_fixedResetBtn);
    box->addLayout(btnRow);
    box->addWidget(makeHint(QString::fromUtf8("重置只清空界面输入，不改变平台实际姿态；需要归零时请再点击执行。"), page));
    box->addStretch();

    connect(m_fixedExecuteBtn, &QPushButton::clicked, this, &PlatformPanel::sendFixedTarget);
    connect(m_fixedResetBtn, &QPushButton::clicked, this, [this] {
        for (auto& row : m_targetRows) {
            if (row.spin) row.spin->setValue(0.0);
        }
    });
    return page;
}

QWidget* PlatformPanel::buildIncrementPage(QWidget* parent)
{
    auto* page = new QWidget(parent);
    auto* box = new QVBoxLayout(page);
    box->setContentsMargins(14, 8, 14, 8);
    box->setSpacing(8);
    box->addWidget(makeSectionTitle(QString::fromUtf8("增量运动"), page));

    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(6);
    for (int i = 0; i < 6; ++i) {
        auto* axis = new QLabel(kAxisNames[i], page);
        axis->setStyleSheet(kCaption);
        m_incrementMinusBtns[i] = new QPushButton("-", page);
        m_incrementPlusBtns[i] = new QPushButton("+", page);
        m_incrementMinusBtns[i]->setStyleSheet(kButton);
        m_incrementPlusBtns[i]->setStyleSheet(kButton);
        m_incrementValueLabels[i] = new QLabel("0.00000", page);
        m_incrementValueLabels[i]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_incrementValueLabels[i]->setStyleSheet(kValue);
        auto* unit = new QLabel(kAxisUnits[i], page);
        unit->setStyleSheet(kCaption);
        grid->addWidget(axis, i, 0);
        grid->addWidget(m_incrementMinusBtns[i], i, 1);
        grid->addWidget(m_incrementValueLabels[i], i, 2);
        grid->addWidget(unit, i, 3);
        grid->addWidget(m_incrementPlusBtns[i], i, 4);
        connect(m_incrementMinusBtns[i], &QPushButton::clicked, this, [this, i] { sendIncrementStep(i, -1.0); });
        connect(m_incrementPlusBtns[i], &QPushButton::clicked, this, [this, i] { sendIncrementStep(i, 1.0); });
    }
    box->addLayout(grid);

    auto* modeRow = new QHBoxLayout;
    m_incrementSingleRadio = new QRadioButton(QString::fromUtf8("单步"), page);
    m_incrementContinuousRadio = new QRadioButton(QString::fromUtf8("连续"), page);
    m_incrementSingleRadio->setChecked(true);
    m_incrementContinuousRadio->setEnabled(false);
    m_incrementContinuousRadio->setToolTip(QString::fromUtf8("连续增量需要设备协议中的速度/停止参数，当前版本不下发该模式。"));
    modeRow->addWidget(m_incrementSingleRadio);
    modeRow->addWidget(m_incrementContinuousRadio);
    modeRow->addStretch();
    box->addLayout(modeRow);

    auto* stepGrid = new QGridLayout;
    m_translateStepSpin = new QDoubleSpinBox(page);
    m_rotateStepSpin = new QDoubleSpinBox(page);
    for (auto* spin : {m_translateStepSpin, m_rotateStepSpin}) {
        spin->setStyleSheet(kSpin);
        spin->setDecimals(5);
        spin->setMinimum(0.00001);
        spin->setMaximum(100.0);
        spin->setValue(0.00001);
        spin->setAlignment(Qt::AlignRight);
    }
    m_translateStepSpin->setSingleStep(1.0);
    m_rotateStepSpin->setSingleStep(0.1);
    stepGrid->addWidget(new QLabel(QString::fromUtf8("平移单步"), page), 0, 0);
    stepGrid->addWidget(m_translateStepSpin, 0, 1);
    stepGrid->addWidget(new QLabel("mm", page), 0, 2);
    stepGrid->addWidget(new QLabel(QString::fromUtf8("旋转单步"), page), 1, 0);
    stepGrid->addWidget(m_rotateStepSpin, 1, 1);
    stepGrid->addWidget(new QLabel("deg", page), 1, 2);
    box->addLayout(stepGrid);

    auto* btnRow = new QHBoxLayout;
    m_incrementStopBtn = new QPushButton(QString::fromUtf8("停止"), page);
    m_incrementResetBtn = new QPushButton(QString::fromUtf8("重置步长"), page);
    m_incrementStopBtn->setStyleSheet(kDangerButton);
    m_incrementResetBtn->setStyleSheet(kButton);
    btnRow->addWidget(m_incrementStopBtn);
    btnRow->addWidget(m_incrementResetBtn);
    box->addLayout(btnRow);
    box->addWidget(makeHint(QString::fromUtf8("单步按钮按当前实时姿态叠加步长，再走姿态定点；连续模式暂不发送，避免协议不完整时误动作。"), page));
    box->addStretch();

    connect(m_incrementStopBtn, &QPushButton::clicked, this, &PlatformPanel::stopMotionRequested);
    connect(m_incrementResetBtn, &QPushButton::clicked, this, [this] {
        if (m_translateStepSpin) m_translateStepSpin->setValue(0.00001);
        if (m_rotateStepSpin) m_rotateStepSpin->setValue(0.00001);
    });
    return page;
}

QWidget* PlatformPanel::buildSinePage(QWidget* parent)
{
    auto* page = new QWidget(parent);
    auto* box = new QVBoxLayout(page);
    box->setContentsMargins(14, 8, 14, 8);
    box->setSpacing(8);
    box->addWidget(makeSectionTitle(QString::fromUtf8("正弦运动"), page));

    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(6);
    grid->setVerticalSpacing(5);
    grid->addWidget(new QLabel(QString::fromUtf8("轴"), page), 0, 0);
    grid->addWidget(new QLabel(QString::fromUtf8("幅值"), page), 0, 1);
    grid->addWidget(new QLabel(QString::fromUtf8("周期"), page), 0, 2);
    grid->addWidget(new QLabel(QString::fromUtf8("相位"), page), 0, 3);
    grid->addWidget(new QLabel(QString::fromUtf8("偏距"), page), 0, 4);
    for (int i = 0; i < 6; ++i) {
        auto* check = new QCheckBox(kAxisNames[i], page);
        check->setEnabled(false);
        grid->addWidget(check, i + 1, 0);
        for (int col = 1; col <= 4; ++col) {
            auto* spin = new QDoubleSpinBox(page);
            spin->setStyleSheet(kSpin);
            spin->setDecimals(3);
            spin->setRange(-1000.0, 1000.0);
            spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
            spin->setEnabled(false);
            grid->addWidget(spin, i + 1, col);
        }
    }
    box->addLayout(grid);

    auto* coord = new QComboBox(page);
    coord->setStyleSheet(kCombo);
    coord->addItem(QString::fromUtf8("世界坐标系"));
    coord->addItem(QString::fromUtf8("平台坐标系"));
    coord->setEnabled(false);
    box->addWidget(coord);

    auto* btnRow = new QHBoxLayout;
    m_sineExecuteBtn = new QPushButton(QString::fromUtf8("执行"), page);
    m_sineResetBtn = new QPushButton(QString::fromUtf8("重置"), page);
    m_sineExecuteBtn->setStyleSheet(kPrimaryButton);
    m_sineResetBtn->setStyleSheet(kButton);
    m_sineExecuteBtn->setEnabled(false);
    m_sineResetBtn->setEnabled(false);
    btnRow->addWidget(m_sineExecuteBtn);
    btnRow->addWidget(m_sineResetBtn);
    box->addLayout(btnRow);
    box->addWidget(makeHint(QString::fromUtf8("界面按厂家手册保留参数结构；正弦参数功能码尚未接入，当前不会下发正弦运动。"), page));
    box->addStretch();
    return page;
}

QWidget* PlatformPanel::buildTrajectoryPage(QWidget* parent)
{
    auto* page = new QWidget(parent);
    auto* box = new QVBoxLayout(page);
    box->setContentsMargins(14, 8, 14, 8);
    box->setSpacing(10);
    box->addWidget(makeSectionTitle(QString::fromUtf8("轨迹运动"), page));

    auto* importRow = new QHBoxLayout;
    auto* importBtn = new QPushButton(QString::fromUtf8("导入 CSV"), page);
    importBtn->setStyleSheet(kButton);
    importBtn->setEnabled(false);
    auto* period = new QDoubleSpinBox(page);
    period->setStyleSheet(kSpin);
    period->setRange(20, 1000);
    period->setValue(20);
    period->setSuffix(" ms");
    period->setEnabled(false);
    importRow->addWidget(importBtn);
    importRow->addStretch();
    importRow->addWidget(new QLabel(QString::fromUtf8("周期"), page));
    importRow->addWidget(period);
    box->addLayout(importRow);
    box->addWidget(makeHint(QString::fromUtf8("CSV 每行按 Tx, Ty, Tz, Rx, Ry, Rz 排列；轨迹发送、循环判断和首点姿态校验还需要协议和文件模块配合。"), page));

    auto* preview = new QWidget(page);
    preview->setStyleSheet("background:#F8FAFC;border:1px solid #E5E7EB;border-radius:5px;");
    auto* previewBox = new QVBoxLayout(preview);
    previewBox->setContentsMargins(10, 8, 10, 8);
    previewBox->addWidget(new QLabel(QString::fromUtf8("文件名：未导入"), preview));
    for (int i = 0; i < 6; ++i) {
        auto* label = new QLabel(QString("%1 (%2): 0.00000").arg(kAxisNames[i], kAxisUnits[i]), preview);
        label->setStyleSheet(kMuted);
        previewBox->addWidget(label);
    }
    box->addWidget(preview);

    auto* btnRow = new QHBoxLayout;
    m_trajectoryStartBtn = new QPushButton(QString::fromUtf8("开始执行"), page);
    m_trajectoryStopBtn = new QPushButton(QString::fromUtf8("停止执行"), page);
    m_trajectoryStartBtn->setStyleSheet(kPrimaryButton);
    m_trajectoryStopBtn->setStyleSheet(kDangerButton);
    m_trajectoryStartBtn->setEnabled(false);
    m_trajectoryStopBtn->setEnabled(false);
    btnRow->addWidget(m_trajectoryStartBtn);
    btnRow->addWidget(m_trajectoryStopBtn);
    box->addStretch();
    box->addLayout(btnRow);
    return page;
}

void PlatformPanel::selectMotionPage(int pageIndex)
{
    if (!m_motionStack) return;
    m_motionStack->setCurrentIndex(pageIndex);
    if (m_modeButtons && m_modeButtons->button(pageIndex)) {
        m_modeButtons->button(pageIndex)->setChecked(true);
    }
}

void PlatformPanel::updateTargetRanges()
{
    for (int i = 0; i < 6; ++i) {
        if (!m_targetRows[i].spin) continue;
        m_targetRows[i].spin->setRange(kTargetMin[i], kTargetMax[i]);
        m_targetRows[i].spin->setSingleStep(i < 3 ? 1.0 : 0.1);
        m_targetRows[i].spin->setValue(0.0);
    }
}

void PlatformPanel::updateButtonStates()
{
    for (auto* btn : {m_powerOnBtn, m_powerOffBtn, m_faultResetBtn,
                      m_fixedExecuteBtn, m_fixedResetBtn,
                      m_incrementStopBtn, m_incrementResetBtn}) {
        if (btn) btn->setEnabled(m_online);
    }
    for (auto& row : m_targetRows) {
        if (row.spin) row.spin->setEnabled(m_online);
    }
    for (auto* btn : m_incrementMinusBtns) {
        if (btn) btn->setEnabled(m_online);
    }
    for (auto* btn : m_incrementPlusBtns) {
        if (btn) btn->setEnabled(m_online);
    }
    if (m_translateStepSpin) m_translateStepSpin->setEnabled(m_online);
    if (m_rotateStepSpin) m_rotateStepSpin->setEnabled(m_online);
    if (m_incrementSingleRadio) m_incrementSingleRadio->setEnabled(m_online);
    if (m_incrementContinuousRadio) m_incrementContinuousRadio->setEnabled(false);
    if (m_coordCombo) m_coordCombo->setEnabled(m_online);
}

void PlatformPanel::updatePoseReadouts()
{
    const double vals[6] = {m_currentPose.x, m_currentPose.y, m_currentPose.z,
                            m_currentPose.rx, m_currentPose.ry, m_currentPose.rz};
    for (int i = 0; i < 6; ++i) {
        const QString text = QString::number(vals[i], 'f', 5);
        if (m_poseRows[i].value) m_poseRows[i].value->setText(text);
        if (m_incrementValueLabels[i]) m_incrementValueLabels[i]->setText(text);
    }
}

void PlatformPanel::sendFixedTarget()
{
    if (!m_online) return;
    emit setMotionModeRequested(static_cast<int>(MotionMode::PoseTarget));
    emit sendPoseTargetRequested(
        m_targetRows[0].spin->value(), m_targetRows[1].spin->value(),
        m_targetRows[2].spin->value(), m_targetRows[3].spin->value(),
        m_targetRows[4].spin->value(), m_targetRows[5].spin->value());
}

void PlatformPanel::sendIncrementStep(int axis, double direction)
{
    if (!m_online || axis < 0 || axis >= 6) return;
    const double step = axis < 3
        ? (m_translateStepSpin ? m_translateStepSpin->value() : 0.0)
        : (m_rotateStepSpin ? m_rotateStepSpin->value() : 0.0);
    if (step <= 0.0) return;

    double vals[6] = {m_currentPose.x, m_currentPose.y, m_currentPose.z,
                      m_currentPose.rx, m_currentPose.ry, m_currentPose.rz};
    vals[axis] = qBound(kTargetMin[axis], vals[axis] + direction * step, kTargetMax[axis]);

    emit setMotionModeRequested(static_cast<int>(MotionMode::PoseTarget));
    emit sendPoseTargetRequested(vals[0], vals[1], vals[2], vals[3], vals[4], vals[5]);
}

QString PlatformPanel::modeName(int mode)
{
    switch (mode) {
    case 0:  return QString::fromUtf8("停止");
    case 1:  return QString::fromUtf8("支链点动");
    case 2:  return QString::fromUtf8("姿态点动");
    case 3:  return QString::fromUtf8("姿态定点");
    case 4:  return QString::fromUtf8("正弦");
    case 5:  return QString::fromUtf8("增量");
    case 6:  return QString::fromUtf8("跟随");
    case 10: return QString::fromUtf8("支链定点");
    default: return QString::fromUtf8("模式%1").arg(mode);
    }
}

void PlatformPanel::onConnectionChanged(bool online)
{
    m_online = online;
    if (m_connIndicator) {
        m_connIndicator->setText(online ? QString::fromUtf8("在线") : QString::fromUtf8("离线"));
        m_connIndicator->setStyleSheet(online
            ? "background:#DCFCE7;color:#166534;border-radius:4px;padding:3px 8px;font-size:8pt;font-weight:bold;"
            : "background:#F1F5F9;color:#64748B;border-radius:4px;padding:3px 8px;font-size:8pt;");
    }
    if (!online) {
        m_currentPose = PlatformPose{};
        if (m_powerState) m_powerState->setText("--");
        if (m_modeDisplay) m_modeDisplay->setText("--");
        if (m_faultIndicator) m_faultIndicator->setText(QString::fromUtf8("正常"));
        updatePoseReadouts();
    }
    updateButtonStates();
}

void PlatformPanel::onPlatformStateUpdated(const PlatformState& state)
{
    m_currentPose = state.currentPose;

    if (m_powerState) {
        m_powerState->setText(state.powered ? QString::fromUtf8("已上电") : QString::fromUtf8("未上电"));
        m_powerState->setStyleSheet(state.powered
            ? "background:#DCFCE7;color:#166534;border-radius:4px;padding:3px 8px;font-size:8pt;font-weight:bold;"
            : "background:#F1F5F9;color:#64748B;border-radius:4px;padding:3px 8px;font-size:8pt;");
    }
    if (m_faultIndicator) {
        m_faultIndicator->setText(state.faultActive ? QString::fromUtf8("故障") : QString::fromUtf8("正常"));
        m_faultIndicator->setStyleSheet(state.faultActive
            ? "background:#FEE2E2;color:#991B1B;border-radius:4px;padding:3px 8px;font-size:8pt;font-weight:bold;"
            : "background:#F1F5F9;color:#64748B;border-radius:4px;padding:3px 8px;font-size:8pt;");
    }
    if (m_modeDisplay) {
        m_modeDisplay->setText(modeName(state.motionMode));
    }
    updatePoseReadouts();
}
