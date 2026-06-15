#include "SensorPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFrame>

// 海军风格颜色（与主窗口一致）
static const char* kGroupStyle =
    "QGroupBox {"
    "  border: 1px solid #0B5EA8;"
    "  border-radius: 6px;"
    "  margin-top: 1.4em;"
    "  background-color: #041830;"
    "  color: #0099CC;"
    "  font-weight: bold;"
    "  font-size: 11pt;"
    "}"
    "QGroupBox::title {"
    "  subcontrol-origin: margin;"
    "  padding: 0 6px;"
    "}";

SensorPanel::SensorPanel(QWidget* parent)
    : QWidget(parent)
{
    m_rows.resize(SensorData::COUNT);
    setupUi();
}

void SensorPanel::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(6);

    auto* group = new QGroupBox("拉线位移传感器  (BRT38 SSI)", this);
    group->setStyleSheet(kGroupStyle);

    auto* vbox = new QVBoxLayout(group);
    vbox->setContentsMargins(10, 14, 10, 10);
    vbox->setSpacing(6);

    // 表头
    auto* headerRow = new QHBoxLayout;
    auto mkHeader = [&](const QString& text, int stretch) -> QLabel* {
        auto* lbl = new QLabel(text, this);
        lbl->setStyleSheet("color: #5B9AC4; font-size: 9pt;");
        lbl->setAlignment(Qt::AlignCenter);
        headerRow->addWidget(lbl, stretch);
        return lbl;
    };
    mkHeader("名称", 4);
    mkHeader("数值", 3);
    mkHeader("单位", 2);
    vbox->addLayout(headerRow);

    // 分隔线
    auto addSep = [&]() {
        auto* f = new QFrame;
        f->setFrameShape(QFrame::HLine);
        f->setStyleSheet("color: #0B5EA8;");
        vbox->addWidget(f);
    };
    addSep();

    // 每个拉线传感器通道行（BRT38 SSI，6路）
    static const char* defaultNames[SensorData::COUNT] = {
        "拉线 1", "拉线 2", "拉线 3",
        "拉线 4", "拉线 5", "拉线 6"
    };
    static const char* defaultUnits[SensorData::COUNT] = {
        "mm", "mm", "mm", "mm", "mm", "mm"
    };

    for (int i = 0; i < SensorData::COUNT; ++i) {
        auto* row = new QHBoxLayout;
        row->setSpacing(4);

        auto& ch = m_rows[i];

        ch.nameLabel = new QLabel(QString::fromUtf8(defaultNames[i]), this);
        ch.nameLabel->setStyleSheet("color: #A8D8F0; font-size: 10pt;");
        ch.nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        ch.valueLabel = new QLabel("---", this);
        ch.valueLabel->setStyleSheet(
            "color: #A8D8F0;"
            "font-family: Consolas, monospace;"
            "font-size: 13pt;"
            "font-weight: bold;"
            "background: #020D1A;"
            "border: 1px solid #0B5EA8;"
            "border-radius: 3px;"
            "padding: 2px 6px;");
        ch.valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ch.valueLabel->setMinimumWidth(80);

        ch.unitLabel = new QLabel(QString::fromUtf8(defaultUnits[i]), this);
        ch.unitLabel->setStyleSheet("color: #5B9AC4; font-size: 9pt;");
        ch.unitLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ch.unitLabel->setFixedWidth(36);

        row->addWidget(ch.nameLabel, 4);
        row->addWidget(ch.valueLabel, 3);
        row->addWidget(ch.unitLabel, 2);
        vbox->addLayout(row);

        if (i < SensorData::COUNT - 1) addSep();
    }

    vbox->addStretch();
    mainLayout->addWidget(group, 1);
}

void SensorPanel::onSensorDataUpdated(const SensorData& data)
{
    for (int i = 0; i < SensorData::COUNT; ++i) {
        const auto& ch = data.channels[i];
        if (!ch.name.isEmpty())
            m_rows[i].nameLabel->setText(ch.name);
        if (!ch.unit.isEmpty())
            m_rows[i].unitLabel->setText(ch.unit);

        if (ch.valid)
            m_rows[i].valueLabel->setText(
                QString::number(static_cast<double>(ch.displayValue), 'f', 2));
        else
            m_rows[i].valueLabel->setText("---");
    }
}

void SensorPanel::onConnectionChanged(bool connected)
{
    for (auto& row : m_rows) {
        if (!connected)
            row.valueLabel->setText("---");
    }
}
