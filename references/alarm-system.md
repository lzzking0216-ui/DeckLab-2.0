# 报警与故障处理 — 完整参考

## 数据结构

```cpp
// alarm/AlarmManager.h
enum class AlarmLevel { Info, Warning, Error, Critical };

struct AlarmEvent {
    qint64     timestamp;       // ms since epoch
    AlarmLevel level;
    QString    source;          // 例: "轴2-SV660N", "GL20-DI3", "Modbus"
    QString    description;
    quint16    rawCode = 0;     // 驱动器原始故障码（0 = 非驱动器报警）
    bool       acknowledged = false;
    QString    id;              // UUID，用于持久化与确认匹配
};
```

---

## 报警分级与触发条件

| 级别 | 颜色 | 典型触发条件 | 行为 |
|------|------|------------|------|
| INFO | 蓝 `#1565C0` | 通信恢复、轴状态切换 | 记录日志，不打断操作 |
| WARNING | 黄 `#E65100` | 传感器值接近限位（80%告警线）| 状态栏提示 |
| ERROR | 橙 `#B71C1C` | SV660N 驱动器故障码非零 | 弹出通知，需手动确认 |
| CRITICAL | 红 `#FF0000` | Modbus 通信中断 / 急停触发 / 多轴同时故障 | 弹窗 + 自动全轴急停 + 声音 |

---

## AlarmManager — 规则引擎

```cpp
// alarm/AlarmManager.h

struct AlarmRule {
    QString     sourceKey;       // 匹配来源标识
    double      minValue;
    double      maxValue;
    AlarmLevel  level;
    QString     descriptionTpl;  // 支持 {value} {axis} 占位符
    int         debounceMs = 300;
};

class AlarmManager : public QObject {
    Q_OBJECT
public:
    explicit AlarmManager(QObject* parent = nullptr);

    void loadRules(const QString& configPath);

    // 评估传感器数据（PlcWorker 每帧调用）
    void evaluateSensor(const SensorData& data);
    // 评估电机状态（每轴每帧调用）
    void evaluateMotor(const MotorData& data);
    // Modbus 通信状态变化
    void onConnectionChanged(bool connected);

    void acknowledge(const QString& id);
    void acknowledgeAll();
    int  unacknowledgedCount() const;
    QVector<AlarmEvent> history() const { return m_history; }

signals:
    void alarmTriggered(AlarmEvent ev);
    void alarmAcknowledged(QString id);
    void unackCountChanged(int count);
    void criticalAlarm(AlarmEvent ev);  // 连接到 PlcWorker 触发全轴急停

private:
    QVector<AlarmRule>  m_rules;
    QVector<AlarmEvent> m_history;
    QMap<QString, qint64> m_lastTriggerTime;  // 防抖
    bool m_connectionLost = false;

    void trigger(AlarmLevel level, const QString& source,
                 const QString& desc, quint16 rawCode = 0);
    void handleCritical(const AlarmEvent& ev);
};
```

```cpp
// alarm/AlarmManager.cpp — 关键实现

void AlarmManager::evaluateMotor(const MotorData& data) {
    QString src = QString("轴%1").arg(data.axisId);

    // 1. 驱动器故障码非零 → ERROR
    if (data.errorCode != 0 && data.faultActive) {
        QString desc = QString("驱动器故障: %1 (0x%2)")
            .arg(faultCodeToString(data.errorCode))
            .arg(data.errorCode, 4, 16, QChar('0'));
        trigger(AlarmLevel::Error, src, desc, data.errorCode);
    }
}

void AlarmManager::onConnectionChanged(bool connected) {
    if (!connected && !m_connectionLost) {
        m_connectionLost = true;
        trigger(AlarmLevel::Critical, "Modbus", "PLC 通信中断，所有轴已急停");
    } else if (connected && m_connectionLost) {
        m_connectionLost = false;
        trigger(AlarmLevel::Info, "Modbus", "PLC 通信已恢复");
    }
}

void AlarmManager::trigger(AlarmLevel level, const QString& source,
                            const QString& desc, quint16 rawCode) {
    // 防抖
    QString key = source + desc;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastTriggerTime.value(key, 0) < 300) return;
    m_lastTriggerTime[key] = now;

    AlarmEvent ev;
    ev.timestamp   = now;
    ev.level       = level;
    ev.source      = source;
    ev.description = desc;
    ev.rawCode     = rawCode;
    ev.id          = QUuid::createUuid().toString(QUuid::WithoutBraces);

    m_history.prepend(ev);
    emit alarmTriggered(ev);
    emit unackCountChanged(unacknowledgedCount());

    if (level == AlarmLevel::Critical)
        handleCritical(ev);
}

void AlarmManager::handleCritical(const AlarmEvent& ev) {
    QApplication::beep();
    emit criticalAlarm(ev);  // PlcWorker 接收后对所有轴写 Quick Stop
}
```

---

## AlarmPanel Widget

```cpp
// alarm/AlarmPanel.cpp

void AlarmPanel::setupUi() {
    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels(
        {"时间", "级别", "来源", "描述", "故障码", "已确认"});
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget { background: #1A1A1A; color: white; gridline-color: #333; }"
        "QTableWidget::item:alternate { background: #222; }"
        "QHeaderView::section { background: #2B2B2B; color: white; }"
    );

    m_ackAllBtn = new QPushButton("全部确认", this);
    m_exportBtn = new QPushButton("导出 CSV", this);

    // 未确认报警计数徽章（嵌在 Tab 标题或顶栏）
    m_unackBadge = new QLabel("0", this);
    m_unackBadge->setStyleSheet(
        "QLabel { background: #CC0000; color: white; border-radius: 10px;"
        " padding: 2px 6px; font-weight: bold; }"
    );
    m_unackBadge->setVisible(false);
}

void AlarmPanel::onAlarm(const AlarmEvent& ev) {
    static const QMap<AlarmLevel, QColor> bgColors = {
        {AlarmLevel::Info,     QColor("#0D2137")},
        {AlarmLevel::Warning,  QColor("#2D1A00")},
        {AlarmLevel::Error,    QColor("#2A0000")},
        {AlarmLevel::Critical, QColor("#3A0000")},
    };
    static const QMap<AlarmLevel, QString> levelNames = {
        {AlarmLevel::Info,     "INFO"},
        {AlarmLevel::Warning,  "WARNING"},
        {AlarmLevel::Error,    "ERROR"},
        {AlarmLevel::Critical, "CRITICAL"},
    };

    m_table->insertRow(0);
    QColor bg = bgColors.value(ev.level, QColor("#1A1A1A"));
    auto setCell = [&](int col, const QString& text) {
        auto* item = new QTableWidgetItem(text);
        item->setBackground(bg);
        item->setForeground(Qt::white);
        m_table->setItem(0, col, item);
    };

    setCell(0, QDateTime::fromMSecsSinceEpoch(ev.timestamp).toString("HH:mm:ss.zzz"));
    setCell(1, levelNames.value(ev.level));
    setCell(2, ev.source);
    setCell(3, ev.description);
    setCell(4, ev.rawCode ? QString("0x%1").arg(ev.rawCode, 4, 16, QChar('0')) : "-");
    setCell(5, "");

    if (ev.level == AlarmLevel::Critical)
        emit criticalAlarmDisplayed();
}

void AlarmPanel::exportCsv() {
    QString path = QFileDialog::getSaveFileName(
        this, "导出报警记录", "", "CSV (*.csv)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream s(&f);
    s << "时间戳,级别,来源,描述,故障码,已确认\n";

    for (int row = 0; row < m_table->rowCount(); ++row) {
        QStringList cols;
        for (int col = 0; col < m_table->columnCount(); ++col)
            cols << m_table->item(row, col)->text();
        s << cols.join(",") << "\n";
    }
    QMessageBox::information(this, "导出完成", path);
}
```

---

## 默认报警规则配置（JSON）

```json
{
  "alarm_rules": [
    {
      "sourceKey": "pressure",
      "minValue": 0,
      "maxValue": 500,
      "level": "Error",
      "descriptionTpl": "压力超限: {value} kPa（范围 0~500）",
      "debounceMs": 500
    },
    {
      "sourceKey": "temperature",
      "minValue": -10,
      "maxValue": 80,
      "level": "Warning",
      "descriptionTpl": "温度异常: {value} °C",
      "debounceMs": 1000
    }
  ]
}
```
