#include "ui_helpers.h"

#include <QDateTime>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>

#include "blockchain.h"
#include "smartContract.h"

#include <algorithm>
#include <optional>

QString applicationStyle() {
    return R"(
QWidget { color: #172B4D; font-family: "Inter", "Segoe UI", "Noto Sans", sans-serif; font-size: 14px; }
QMainWindow, QWidget#content, QStackedWidget { background: #F6F8FC; }
QLabel { background: transparent; }
QFrame#sidebar { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0B1F3A, stop:1 #102A43); }
QLabel#brand { color: white; background: transparent; font-size: 21px; font-weight: 800; letter-spacing: 0.4px; }
QLabel#brandSub { color: #9FB3C8; background: transparent; font-size: 10px; font-weight: 600; letter-spacing: 1.2px; }
QPushButton#nav { background: transparent; border: 1px solid transparent; border-radius: 10px; color: #C9D6E2; padding: 12px 14px; text-align: left; font-size: 14px; }
QPushButton#nav:hover { background: rgba(255,255,255,0.08); color: white; }
QPushButton#nav:checked { background: #1D5E93; border-color: #357AAF; color: white; font-weight: 700; }
QLabel#pageTitle { font-size: 28px; font-weight: 800; color: #102A43; }
QLabel#muted { color: #486581; }
QFrame#card { background: white; border: 1px solid #E3EAF3; border-radius: 14px; }
QLabel#cardValue { color: #102A43; font-size: 27px; font-weight: 800; }
QLabel#cardLabel { color: #486581; font-size: 12px; font-weight: 600; }
QLabel#status { background: #DCFCE7; color: #166534; border-radius: 10px; padding: 6px 10px; font-size: 11px; font-weight: 800; }
QLabel#statusError { background: #FEE2E2; color: #B91C1C; border-radius: 10px; padding: 6px 10px; font-size: 11px; font-weight: 800; }
QPushButton { background: #1677C8; color: white; border: 0; border-radius: 9px; padding: 10px 16px; font-weight: 700; }
QPushButton:hover { background: #0F65AA; }
QPushButton:pressed { background: #0B4F87; }
QPushButton:focus, QLineEdit:focus, QComboBox:focus, QTableWidget:focus { border: 2px solid #48A9E6; }
QPushButton:disabled { background: #CBD5E1; color: #52606D; }
QPushButton#secondary { background: white; color: #1265AA; border: 1px solid #B9D8F2; }
QPushButton#secondary:hover { background: #EDF7FF; border-color: #76B9E6; }
QLineEdit, QComboBox { background: white; border: 1px solid #D9E2EC; border-radius: 9px; padding: 9px 11px; min-height: 18px; }
QLineEdit:hover, QComboBox:hover { border-color: #9FB3C8; }
QLineEdit:focus, QComboBox:focus { border: 2px solid #48A9E6; }
QTableWidget { background: white; border: 1px solid #E3EAF3; border-radius: 14px; gridline-color: #EEF2F6; selection-background-color: #E3F2FD; selection-color: #102A43; }
QHeaderView::section { background: #F8FAFC; border: 0; border-bottom: 1px solid #E3EAF3; color: #486581; padding: 12px; font-size: 12px; font-weight: 700; }
QTableWidget::item { padding: 9px; border-bottom: 1px solid #F0F4F8; }
QTableWidget::item:selected { color: #102A43; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 8px 3px; }
QScrollBar::handle:vertical { background: #829AB1; border-radius: 4px; min-height: 28px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QDialog { background: #F6F8FC; }
QLabel#tag { background: #E8F4FC; color: #1265AA; border-radius: 10px; padding: 4px 8px; font-size: 11px; font-weight: 700; }
QLabel#successTag { background: #DCFCE7; color: #166534; border-radius: 10px; padding: 5px 9px; font-size: 11px; font-weight: 800; }
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
    result->setTextElideMode(Qt::ElideNone);
    result->setWordWrap(true);
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
            auto *item = new QTableWidgetItem(values[col]);
            item->setToolTip(values[col]);
            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            target->setItem(row, col, item);
        }
        target->setRowHeight(row, 48);
    }
    target->resizeRowsToContents();
    for (int row = 0; row < target->rowCount(); ++row) {
        target->setRowHeight(row, std::max(48, target->rowHeight(row)));
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
