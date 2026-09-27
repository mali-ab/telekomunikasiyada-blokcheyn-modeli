#include "main_window.h"

#include "ui_helpers.h"

#include <QDate>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QStyle>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "blockchain.h"
#include "database.h"
#include "signature.h"
#include "smartContract.h"
#include "subscriber_repository.h"

#include <memory>
#include <vector>

MainWindow::MainWindow() {
    setWindowTitle("TürkmenTelekom • Blockchain Operator");
    setMinimumSize(1080, 680);
    resize(1320, 800);
    try {
        DatabaseManager::initTables();
        blockchain_ = std::make_unique<Blockchain>();
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Database", QString("Could not connect to PostgreSQL:\n%1").arg(e.what()));
    }
    build();
    refreshAll();
}

MainWindow::~MainWindow() = default;

QWidget *MainWindow::cardWithValue(const QString &value, const QString &caption, const QString &detail, QLabel **valueOut) {
    auto *box = new QFrame;
    box->setObjectName("card");
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(5);
    auto *valueLabel = label(value, "cardValue");
    *valueOut = valueLabel;
    layout->addWidget(valueLabel);
    layout->addWidget(label(caption, "cardLabel"));
    layout->addWidget(label(detail, "muted"));
    return box;
}

QWidget *MainWindow::pageHeader(const QString &title, const QString &subtitle, QWidget *right) {
    auto *header = new QWidget;
    auto *layout = new QHBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 12);
    auto *words = new QVBoxLayout;
    words->setSpacing(3);
    words->addWidget(label(title, "pageTitle"));
    words->addWidget(label(subtitle, "muted"));
    layout->addLayout(words);
    layout->addStretch();
    if (right) layout->addWidget(right);
    return header;
}

void MainWindow::refreshAll() {
    refreshSubscribers();
    refreshBlocks();
    refreshDashboard();
    if (nodeStatus_) {
        const bool connected = blockchain_ && DatabaseManager::isConnected();
        nodeStatus_->setText(connected ? "●  DB CONNECTED" : "●  DB OFFLINE");
        nodeStatus_->setObjectName(connected ? "status" : "statusError");
        nodeStatus_->style()->unpolish(nodeStatus_);
        nodeStatus_->style()->polish(nodeStatus_);
    }
}

int MainWindow::countSubscribers() {
    try {
        return SubscriberRepository::count();
    } catch (...) {
        return 0;
    }
}

void MainWindow::refreshDashboard() {
    if (!subscriberMetric_) return;
    subscriberMetric_->setText(QString::number(countSubscribers()));
    if (!blockchain_) return;
    const auto blocks = blockchain_->getAllBlocks();
    if (blockMetric_) blockMetric_->setText(QString::number(blocks.size()));

    const QDate today = QDate::currentDate();
    double todayTotal = 0;
    int todayCount = 0;
    QList<QStringList> rows;
    for (int i = static_cast<int>(blocks.size()) - 1; i >= 0; --i) {
        const Block &block = blocks[static_cast<size_t>(i)];
        if (block.sData == "Genesis Block") continue;
        const auto payment = paymentFromBlockData(block.sData);
        if (payment && QDateTime::fromSecsSinceEpoch(static_cast<qint64>(block.tTime)).date() == today) {
            todayTotal += *payment;
            ++todayCount;
        }
        if (rows.size() == 8) continue;
        const auto subscriber = fieldFromBlockData(block.sData, "Subscriber: ");
        const auto service = fieldFromBlockData(block.sData, "Service: ");
        rows.append({subscriber ? QString::fromStdString(*subscriber) : "—", service ? QString::fromStdString(*service) : "—",
                     payment ? QString("%1 TMT").arg(*payment, 0, 'f', 0) : "—", QString("#%1").arg(block.getIndex()), "Confirmed"});
    }
    if (paymentsMetric_) paymentsMetric_->setText(QString("%1 TMT").arg(todayTotal, 0, 'f', 0));
    if (paymentsDetail_) paymentsDetail_->setText(QString("%1 confirmed today").arg(todayCount));
    if (recentTable_) setTableRows(recentTable_, rows);
}

void MainWindow::refreshSubscribers() {
    if (!subscriberTable_) return;
    QList<QStringList> rows;
    try {
        const auto subscribers = SubscriberRepository::list();
        for (size_t index = 0; index < subscribers.size(); ++index) {
            const auto &subscriber = subscribers[index];
            rows.append({QString::number(index + 1), QString::fromStdString(subscriber.name), internetLabel(subscriber), iptvLabel(subscriber),
                         phoneLabel(subscriber), QString("%1 TMT").arg(subscriber.balance, 0, 'f', 0), subscriberStatusLabel(subscriber)});
        }
    } catch (const std::exception &e) {
        showToast(QString("Subscriber load failed: %1").arg(e.what()), true);
    }
    setTableRows(subscriberTable_, rows);
}

void MainWindow::refreshBlocks() {
    if (!blockTable_ || !blockchain_) return;
    QList<QStringList> rows;
    const auto blocks = blockchain_->getAllBlocks();
    for (int i = static_cast<int>(blocks.size()) - 1; i >= 0; --i) {
        const Block &block = blocks[static_cast<size_t>(i)];
        rows.append({QString("#%1").arg(block.getIndex()), formatBlockTime(block.tTime), blockSummary(block), truncateHash(block.sPrevHash),
                     block.sData == "Genesis Block" ? "Genesis" : "Verified"});
    }
    setTableRows(blockTable_, rows);
    if (blockCount_) blockCount_->setText(QString("%1 blocks • SHA-256 • RSA verified").arg(blocks.size()));
}

bool MainWindow::processPayment(const QString &subscriberName, ServiceType service, double amount, QString *errorOut) {
    if (!blockchain_) { if (errorOut) *errorOut = "Blockchain is not available."; return false; }
    const std::string name = subscriberName.trimmed().toStdString();
    if (name.empty()) { if (errorOut) *errorOut = "Enter a subscriber name."; return false; }
    Subscriber subscriber(name, false);
    if (!SmartContract::getSubscriberFromDB(name, subscriber)) {
        SmartContract::updateSubscriberInDB(subscriber);
    }
    const std::string transactionData = subscriber.getTransactionData(service, amount);
    const RSAKeys keys = DigitalSignature::getKeys("keys_" + subscriber.name + ".txt");
    const std::vector<long long> signature = DigitalSignature::encrypt(transactionData, keys.d, keys.n);
    if (DigitalSignature::decrypt(signature, keys.e, keys.n) != transactionData) {
        if (errorOut) *errorOut = "RSA signature verification failed.";
        return false;
    }
    SmartContract::processService(subscriber, amount, service);
    blockchain_->AddBlock(Block(blockchain_->getLatestBlock().getIndex() + 1, transactionData));
    return true;
}

void MainWindow::runAudit() {
    if (!blockchain_) { showToast("Blockchain not loaded.", true); return; }
    const bool ok = blockchain_->fullAudit();
    showToast(ok ? "Integrity audit passed — all blocks are valid." : "Audit failed — chain integrity issue detected.", !ok);
}

void MainWindow::showToast(const QString &message, bool error) {
    toast_->setText(message);
    toast_->setStyleSheet(error ? "color: #B91C1C; font-weight: 600;" : "color: #1F7A3E; font-weight: 600;");
    QTimer::singleShot(4500, this, [this] { toast_->clear(); });
}
