#include "ui_helpers.h"

#include <QDateTime>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>

#include "blockchain.h"
#include "smartContract.h"

#include <optional>

QString applicationStyle() {
    return R"(
QWidget { background: #F5F7FB; color: #14213D; font-family: "Segoe UI", "Noto Sans", sans-serif; font-size: 14px; }
QFrame#sidebar { background: #102A43; }
QLabel#brand { color: white; background: #102A43; font-size: 20px; font-weight: 700; }
QLabel#brandSub { color: #9FB3C8; background: #102A43; font-size: 11px; letter-spacing: 1px; }
QPushButton#nav { background: transparent; border: 0; border-radius: 8px; color: #C9D6E2; padding: 12px 16px; text-align: left; font-size: 14px; }
QPushButton#nav:hover, QPushButton#nav:checked { background: #1F4D72; color: white; font-weight: 600; }
QLabel#pageTitle { font-size: 26px; font-weight: 700; color: #102A43; }
QLabel#muted { color: #627D98; }
QFrame#card { background: white; border: 1px solid #E4EAF1; border-radius: 12px; }
QLabel#cardValue { color: #102A43; font-size: 24px; font-weight: 700; }
QLabel#cardLabel { color: #627D98; font-size: 12px; }
QLabel#status { background: #E3F9E5; color: #1F7A3E; border-radius: 10px; padding: 5px 9px; font-size: 12px; font-weight: 600; }
QLabel#statusError { background: #FEE2E2; color: #B91C1C; border-radius: 10px; padding: 5px 9px; font-size: 12px; font-weight: 600; }
QPushButton { background: #1677C8; color: white; border: 0; border-radius: 8px; padding: 10px 16px; font-weight: 600; }
QPushButton:hover { background: #1265AA; }
QPushButton#secondary { background: white; color: #1677C8; border: 1px solid #B9D8F2; }
QPushButton#secondary:hover { background: #EDF7FF; }
QLineEdit, QComboBox { background: white; border: 1px solid #D9E2EC; border-radius: 8px; padding: 9px 11px; min-height: 18px; }
QLineEdit:focus, QComboBox:focus { border: 2px solid #48A9E6; }
QTableWidget { background: white; border: 1px solid #E4EAF1; border-radius: 12px; gridline-color: #EEF2F6; selection-background-color: #E8F4FC; selection-color: #102A43; }
QHeaderView::section { background: #F8FAFC; border: 0; border-bottom: 1px solid #E4EAF1; color: #627D98; padding: 11px; font-weight: 600; }
QTableWidget::item { padding: 8px; border-bottom: 1px solid #EEF2F6; }
QLabel#tag { background: #E8F4FC; color: #1677C8; border-radius: 10px; padding: 4px 8px; font-size: 11px; font-weight: 600; }
QLabel#successTag { background: #E3F9E5; color: #1F7A3E; border-radius: 10px; padding: 4px 8px; font-size: 11px; font-weight: 600; }
)";
}

QLabel *label(const QString &text, const QString &name) {
    auto *result = new QLabel(text);
    if (!name.isEmpty()) result->setObjectName(name);
    return result;
}

QTableWidget *table(const QStringList &headers) {
    auto *result = new QTableWidget;
    result->setColumnCount(headers.size());
    result->setHorizontalHeaderLabels(headers);
    result->verticalHeader()->hide();
    result->setShowGrid(false);
    result->setEditTriggers(QAbstractItemView::NoEditTriggers);
    result->setSelectionBehavior(QAbstractItemView::SelectRows);
    result->setAlternatingRowColors(false);
    result->horizontalHeader()->setStretchLastSection(true);
    result->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    return result;
}

void setTableRows(QTableWidget *target, const QList<QStringList> &rows) {
    target->setRowCount(0);
    for (const auto &values : rows) {
        const int row = target->rowCount();
        target->insertRow(row);
        for (int col = 0; col < values.size(); ++col) {
            target->setItem(row, col, new QTableWidgetItem(values[col]));
        }
        target->setRowHeight(row, 48);
    }
}

QString truncateHash(const std::string &hash) {
    if (hash.size() <= 16) return QString::fromStdString(hash);
    return QString::fromStdString(hash.substr(0, 8) + "…" + hash.substr(hash.size() - 4));
}

QString formatBlockTime(std::time_t timestamp) {
    if (timestamp <= 0) return "—";
    return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(timestamp)).toString("yyyy-MM-dd hh:mm");
}

QString subscriberStatusLabel(const Subscriber &subscriber) {
    const bool active = subscriber.isServiceActive(ServiceType::INTERNET) ||
                        subscriber.isServiceActive(ServiceType::IPTV) ||
                        subscriber.isServiceActive(ServiceType::PHONE);
    if (active) return "Active";
    if (subscriber.balance > 0.01) return "Pending";
    return "Inactive";
}

QString internetLabel(const Subscriber &subscriber) {
    const QString plan = QString::fromStdString(subscriber.internet.speed) + " Mbit/s";
    if (!subscriber.isServiceActive(ServiceType::INTERNET)) return plan + " • Inactive";
    return plan + QString(" • %1 days left").arg(subscriber.remainingDays(subscriber.internetExpiry));
}

QString iptvLabel(const Subscriber &subscriber) {
    const QString plan = QString("%1 channels").arg(subscriber.iptv.tvCount);
    if (!subscriber.isServiceActive(ServiceType::IPTV)) return plan + " • Inactive";
    return plan + QString(" • %1 days left").arg(subscriber.remainingDays(subscriber.iptvExpiry));
}

QString phoneLabel(const Subscriber &subscriber) {
    if (!subscriber.isServiceActive(ServiceType::PHONE)) return "Inactive";
    return QString("Active • %1 days left").arg(subscriber.remainingDays(subscriber.phoneExpiry));
}

std::optional<std::string> fieldFromBlockData(const std::string &data, const std::string &key) {
    const size_t pos = data.find(key);
    if (pos == std::string::npos) return std::nullopt;
    const size_t start = pos + key.size();
    const size_t end = data.find(',', start);
    if (end == std::string::npos) return data.substr(start);
    return data.substr(start, end - start);
}

std::optional<double> paymentFromBlockData(const std::string &data) {
    const size_t pos = data.find("Payment: ");
    if (pos == std::string::npos) return std::nullopt;
    const size_t tmt = data.find(" TMT", pos);
    if (tmt == std::string::npos) return std::nullopt;
    try {
        return std::stod(data.substr(pos + 9, tmt - (pos + 9)));
    } catch (...) {
        return std::nullopt;
    }
}

QString blockSummary(const Block &block) {
    if (block.sData == "Genesis Block") return "Genesis block";
    const auto subscriber = fieldFromBlockData(block.sData, "Subscriber: ");
    const auto service = fieldFromBlockData(block.sData, "Service: ");
    const auto payment = paymentFromBlockData(block.sData);
    if (!subscriber || !service || !payment) return QString::fromStdString(block.sData);
    return QString("%1 • %2 • %3 TMT")
        .arg(QString::fromStdString(*subscriber))
        .arg(QString::fromStdString(*service))
        .arg(*payment, 0, 'f', 0);
}
