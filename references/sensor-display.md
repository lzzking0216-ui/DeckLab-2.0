# 传感器实时显示 — 完整参考（GL20-RTU-ECT）

## GL20-RTU-ECT 说明

GL20-RTU-ECT 是汇川远程 IO 模块，通过 EtherCAT 连接到 AC702 PLC。
传感器信号经 GL20 采集后，PLC 将其映射到 Modbus 保持寄存器 `SENSOR_BASE`（MW256）起始区域。
**上位机只需读取寄存器，无需了解 EtherCAT 或 GL20 内部细节。**

InProShop 配置时，需将 GL20 的 IO 变量映射到 MW 区，并记录每个通道对应的寄存器地址。

---

## 数据结构

```cpp
// sensor/SensorManager.h
#pragma once
#include <QVector>

// GL20 支持数字量（DI/DO）和模拟量（AI/AO），
// 按实际接线定义通道，以下为示例结构，根据项目扩展
struct SensorData {
    qint64 timestamp;          // ms since epoch

    // 模拟量输入通道（根据 GL20 实际接线填写物理量名称和单位）
    QVector<double> analogIn;   // 按通道顺序排列，单位由工程换算决定

    // 数字量输入通道（对应 GL20 DI 点位）
    QVector<bool> digitalIn;
};

// 通道配置（从配置文件读取，运行时可调）
struct ChannelConfig {
    int     regOffset;      // 相对 SENSOR_BASE 的寄存器偏移
    QString name;           // 通道名，例如 "压力_左", "温度_1"
    QString unit;           // 单位，例如 "kPa", "°C", "mm"
    double  scale;          // 工程量换算系数：工程量 = 寄存器值 × scale + offset
    double  rawOffset;
    double  minAlarm;       // 报警下限
    double  maxAlarm;       // 报警上限
    bool    isDigital;      // true = 数字量，false = 模拟量
};
```

---

## SensorManager 类

```cpp
// sensor/SensorManager.h
class SensorManager : public QObject {
    Q_OBJECT
public:
    explicit SensorManager(QObject* parent = nullptr);

    // 从配置文件加载通道定义
    void loadConfig(const QString& configPath);

    // 由 PlcWorker 调用：传入原始寄存器值，转换后发射信号
    void processRawRegisters(const QVector<quint16>& rawRegs, qint64 timestamp);

    QVector<ChannelConfig> channels() const { return m_channels; }

signals:
    void dataUpdated(SensorData data);

private:
    QVector<ChannelConfig> m_channels;

    double toEngineering(const ChannelConfig& ch, quint16 raw) const {
        return raw * ch.scale + ch.rawOffset;
    }
};
```

```cpp
// sensor/SensorManager.cpp
void SensorManager::processRawRegisters(const QVector<quint16>& rawRegs, qint64 ts) {
    SensorData d;
    d.timestamp = ts;
    d.analogIn.resize(m_channels.size());
    d.digitalIn.resize(m_channels.size());

    for (int i = 0; i < m_channels.size(); ++i) {
        const auto& ch = m_channels[i];
        if (ch.regOffset >= rawRegs.size()) continue;
        quint16 raw = rawRegs[ch.regOffset];
        if (ch.isDigital) {
            d.digitalIn[i] = (raw != 0);
        } else {
            d.analogIn[i] = toEngineering(ch, raw);
        }
    }
    emit dataUpdated(d);
}
```

---

## QCustomPlot 实时曲线

```cpp
// sensor/SensorPanel.cpp

void SensorPanel::initPlot() {
    m_plot = new QCustomPlot(this);
    m_plot->setBackground(QColor("#1E1E1E"));

    // 按通道数动态添加曲线
    static const QList<QColor> palette = {
        Qt::cyan, Qt::yellow, QColor("#00FF88"), QColor("#FF6B6B"),
        QColor("#FFB347"), QColor("#A78BFA"), Qt::white, Qt::magenta
    };

    const auto& channels = m_sensorManager->channels();
    for (int i = 0; i < channels.size(); ++i) {
        if (channels[i].isDigital) continue;  // 数字量不上曲线
        m_plot->addGraph();
        QColor c = palette[i % palette.size()];
        m_plot->graph()->setPen(QPen(c, 1.5));
        m_plot->graph()->setName(
            QString("%1 (%2)").arg(channels[i].name).arg(channels[i].unit));
        m_plot->graph()->setAdaptiveSampling(true);
        m_graphIndexMap[i] = m_plot->graphCount() - 1;  // 通道 → 图形 索引映射
    }

    // 坐标轴深色风格
    auto styleAxis = [](QCPAxis* ax) {
        ax->setBasePen(QPen(Qt::white));
        ax->setTickPen(QPen(Qt::white));
        ax->setSubTickPen(QPen(Qt::gray));
        ax->setTickLabelColor(Qt::white);
        ax->setLabelColor(Qt::white);
    };
    styleAxis(m_plot->xAxis);
    styleAxis(m_plot->yAxis);
    m_plot->xAxis->setLabel("时间 (s)");
    m_plot->legend->setVisible(true);
    m_plot->legend->setBrush(QBrush(QColor(30, 30, 30, 180)));
    m_plot->legend->setTextColor(Qt::white);
    m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
}

void SensorPanel::onDataUpdated(const SensorData& d) {
    double t = d.timestamp / 1000.0;
    const auto& channels = m_sensorManager->channels();

    for (int i = 0; i < channels.size(); ++i) {
        if (channels[i].isDigital) continue;
        int gIdx = m_graphIndexMap.value(i, -1);
        if (gIdx < 0) continue;
        m_plot->graph(gIdx)->addData(t, d.analogIn[i]);
        // 清理 60s 前的旧数据，防止内存膨胀
        m_plot->graph(gIdx)->data()->removeBefore(t - 60.0);
    }

    if (!m_paused) {
        m_plot->xAxis->setRange(t, 30, Qt::AlignRight);  // 30s 滚动窗口
        m_plot->replot(QCustomPlot::rpQueuedReplot);       // 异步重绘，不卡 UI
    }

    // 更新数值显示面板
    updateDigitalPanel(d);
    updateAnalogPanel(d);
}
```

---

## 数字量 IO 显示面板（GL20 DI 点位）

```cpp
// 数字量：用圆形指示灯（自定义 QLed）显示 ON/OFF
void SensorPanel::updateDigitalPanel(const SensorData& d) {
    const auto& channels = m_sensorManager->channels();
    for (int i = 0; i < channels.size(); ++i) {
        if (!channels[i].isDigital) continue;
        bool on = i < d.digitalIn.size() && d.digitalIn[i];
        // QLed: setColor(on ? Qt::green : Qt::gray)
        m_diLeds[i]->setOn(on);
        m_diLabels[i]->setText(on ? "ON" : "OFF");
    }
}
```

---

## 模拟量数值显示（仪表盘风格）

```cpp
QLabel* SensorPanel::makeValueLabel(const QString& unit, QWidget* parent) {
    auto* lbl = new QLabel("---", parent);
    lbl->setFont(QFont("Consolas", 18, QFont::Bold));
    lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lbl->setStyleSheet(
        "QLabel { color: #00FF88; background: #111;"
        " border: 1px solid #333; border-radius: 4px; padding: 4px 10px; }"
    );
    lbl->setToolTip(unit);
    lbl->setMinimumWidth(130);
    return lbl;
}

// 超限时变红
void SensorPanel::updateValueLabel(QLabel* lbl, double val,
                                   double minOk, double maxOk, const QString& unit) {
    lbl->setText(QString("%1 %2").arg(val, 0, 'f', 2).arg(unit));
    bool outOfRange = (val < minOk || val > maxOk);
    lbl->setStyleSheet(outOfRange
        ? "QLabel { color: #FF4444; background: #1A0000;"
          " border: 1px solid #FF4444; border-radius: 4px; padding: 4px 10px; }"
        : "QLabel { color: #00FF88; background: #111;"
          " border: 1px solid #333; border-radius: 4px; padding: 4px 10px; }"
    );
}
```

---

## 暂停 / 继续 / 导出 CSV

```cpp
void SensorPanel::onPauseToggled(bool paused) {
    m_paused = paused;
    m_pauseBtn->setText(paused ? "继续" : "暂停");
    if (!paused) {
        double t = QDateTime::currentMSecsSinceEpoch() / 1000.0;
        m_plot->xAxis->setRange(t, 30, Qt::AlignRight);
        m_plot->replot();
    }
}

void SensorPanel::exportCsv() {
    QString path = QFileDialog::getSaveFileName(
        this, "导出传感器数据", "", "CSV (*.csv)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream s(&f);

    // 表头：时间戳 + 各模拟量通道名
    s << "timestamp_ms";
    for (const auto& ch : m_sensorManager->channels()) {
        if (!ch.isDigital)
            s << "," << ch.name << "(" << ch.unit << ")";
    }
    s << "\n";

    // 以第一条曲线的时间戳为基准对齐数据
    if (m_plot->graphCount() == 0) return;
    auto& ref = *m_plot->graph(0)->data();
    for (auto it = ref.constBegin(); it != ref.constEnd(); ++it) {
        s << QString::number(it->key * 1000, 'f', 0);
        for (int g = 0; g < m_plot->graphCount(); ++g) {
            auto jt = m_plot->graph(g)->data()->findBegin(it->key);
            s << "," << QString::number(jt->value, 'f', 4);
        }
        s << "\n";
    }
    QMessageBox::information(this, "导出完成", path);
}
```

---

## 通道配置文件示例（JSON）

```json
{
  "sensor_channels": [
    {
      "regOffset": 0,
      "name": "压力_左",
      "unit": "kPa",
      "scale": 0.1,
      "rawOffset": 0,
      "minAlarm": 0,
      "maxAlarm": 500,
      "isDigital": false
    },
    {
      "regOffset": 1,
      "name": "温度_1",
      "unit": "°C",
      "scale": 0.01,
      "rawOffset": -273.15,
      "minAlarm": -10,
      "maxAlarm": 80,
      "isDigital": false
    },
    {
      "regOffset": 8,
      "name": "到位信号_1",
      "unit": "",
      "scale": 1,
      "rawOffset": 0,
      "minAlarm": 0,
      "maxAlarm": 1,
      "isDigital": true
    }
  ]
}
```
