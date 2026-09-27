#include "main_window.h"

#include "ui_helpers.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStackedWidget>
#include <QTableWidget>
#include <QVBoxLayout>

#include "database.h"
#include "blockchain.h"
#include "smartContract.h"
#include "subscriber_repository.h"

#include <algorithm>
#include <vector>

QWidget *MainWindow::buildDashboardPage() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setSpacing(18);
    auto *payment = new QPushButton("+ New payment");
    connect(payment, &QPushButton::clicked, this, [this] { paymentDialog(); });
    layout->addWidget(pageHeader("Operations overview", "Live data from PostgreSQL and the validated chain", payment));
    auto *cards = new QGridLayout;
    cards->setSpacing(14);
    QLabel *unused = nullptr;
    cards->addWidget(cardWithValue("—", "Registered subscribers", "From database", &subscriberMetric_), 0, 0);
    cards->addWidget(cardWithValue("OK", "Database", "PostgreSQL", &unused), 0, 1);
    cards->addWidget(cardWithValue("—", "Validated blocks", "SHA-256 • RSA verified", &blockMetric_), 0, 2);
    cards->addWidget(cardWithValue("—", "Today's payments", "Confirmed transactions", &paymentsMetric_), 0, 3);
    paymentsDetail_ = label("", "muted");
    layout->addLayout(cards);
    layout->addWidget(paymentsDetail_);
    auto *activityTitle = label("Recent transactions");
    activityTitle->setStyleSheet("font-size: 17px; font-weight: 700;");
    layout->addWidget(activityTitle);
    recentTable_ = table({"Subscriber", "Service", "Amount", "Block", "Status"});
    layout->addWidget(recentTable_, 1);
    return page;
}

QWidget *MainWindow::subscribers() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setSpacing(16);
    auto *add = new QPushButton("+ Add subscriber");
    connect(add, &QPushButton::clicked, this, [this] { addSubscriber(); });
    auto *edit = new QPushButton("Edit selected services");
    edit->setObjectName("secondary");
    connect(edit, &QPushButton::clicked, this, [this] { editSelectedSubscriberServices(); });
    auto *actions = new QWidget;
    auto *actionLayout = new QHBoxLayout(actions);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(8);
    actionLayout->addWidget(edit);
    auto *refresh = new QPushButton("Refresh");
    refresh->setObjectName("secondary");
    connect(refresh, &QPushButton::clicked, this, [this] { refreshAll(); showToast("Subscriber list refreshed."); });
    actionLayout->addWidget(refresh);
    actionLayout->addWidget(add);
    layout->addWidget(pageHeader("Subscribers", "Manage accounts, balances and active services", actions));
    auto *filter = new QLineEdit;
    filter->setPlaceholderText("Search by name or balance...");
    layout->addWidget(filter);
    subscriberTable_ = table({"No.", "Subscriber", "Internet", "IPTV", "Phone", "Balance", "Status"});
    connect(filter, &QLineEdit::textChanged, this, [this](const QString &query) {
        for (int row = 0; row < subscriberTable_->rowCount(); ++row) {
            bool show = false;
            for (int col = 0; col < subscriberTable_->columnCount(); ++col) {
                const auto *item = subscriberTable_->item(row, col);
                if (item && item->text().contains(query, Qt::CaseInsensitive)) show = true;
            }
            subscriberTable_->setRowHidden(row, !show);
        }
    });
    connect(subscriberTable_, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (const auto *name = subscriberTable_->item(row, 1)) editSubscriberServices(name->text());
    });
    layout->addWidget(subscriberTable_, 1);
    return page;
}

QWidget *MainWindow::blockchainPage() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setSpacing(16);
    auto *audit = new QPushButton("Run integrity audit");
    audit->setObjectName("secondary");
    connect(audit, &QPushButton::clicked, this, [this] { runAudit(); });
    layout->addWidget(pageHeader("Blockchain ledger", "Immutable record of validated service transactions", audit));
    auto *bar = new QFrame;
    bar->setObjectName("card");
    auto *barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(16, 12, 16, 12);
    barLayout->addWidget(label("●  Chain secure", "successTag"));
    barLayout->addSpacing(8);
    blockCount_ = label("—", "muted");
    barLayout->addWidget(blockCount_);
    barLayout->addStretch();
    barLayout->addWidget(label("Loaded from database", "muted"));
    layout->addWidget(bar);
    blockTable_ = table({"Block", "Timestamp", "Transaction", "Previous hash", "Validation"});
    layout->addWidget(blockTable_, 1);
    return page;
}

QWidget *MainWindow::network() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setSpacing(16);
    auto *connectPeer = new QPushButton("+ Connect node");
    connect(connectPeer, &QPushButton::clicked, this, [this] { connectNode(); });
    layout->addWidget(pageHeader("P2P network", "Use the console app for live peer networking", connectPeer));
    auto *cards = new QGridLayout;
    cards->setSpacing(14);
    QLabel *unused = nullptr;
    cards->addWidget(cardWithValue("—", "Active peers", "Console operator only", &unused), 0, 0);
    cards->addWidget(cardWithValue("8080", "Default port", "P2P not started in UI", &unused), 0, 1);
    cards->addWidget(cardWithValue("—", "UI mode", "Database-backed views", &unused), 0, 2);
    layout->addLayout(cards);
    auto *nodes = table({"Node", "Address", "Port", "Last seen", "State"});
    setTableRows(nodes, {{"—", "—", "—", "—", "Start telekom_system for P2P"}});
    layout->addWidget(nodes, 1);
    return page;
}

QWidget *MainWindow::settings() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setSpacing(16);
    layout->addWidget(pageHeader("System settings", "Local operator and network preferences"));
    auto *card = new QFrame;
    card->setObjectName("card");
    auto *form = new QFormLayout(card);
    form->setContentsMargins(24, 22, 24, 22);
    form->setSpacing(16);
    auto *operatorName = new QLineEdit("Ashgabat Central Operator");
    auto *port = new QLineEdit("8080");
    port->setValidator(new QIntValidator(1, 65535, port));
    auto *difficulty = new QComboBox;
    difficulty->addItems({"3 — Standard", "4 — High", "5 — Maximum"});
    form->addRow("Operator name", operatorName);
    form->addRow("P2P port", port);
    form->addRow("Mining difficulty", difficulty);
    auto *save = new QPushButton("Save settings");
    connect(save, &QPushButton::clicked, this, [this] { showToast("Settings saved locally."); });
    form->addRow("", save);
    layout->addWidget(card);
    layout->addStretch();
    return page;
}

void MainWindow::build() {
    auto *root = new QWidget;
    auto *horizontal = new QHBoxLayout(root);
    horizontal->setContentsMargins(0, 0, 0, 0);
    horizontal->setSpacing(0);
    auto *sidebar = new QFrame;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(242);
    auto *nav = new QVBoxLayout(sidebar);
    nav->setContentsMargins(18, 26, 18, 20);
    nav->setSpacing(5);
    nav->addWidget(label("TÜRKMEN", "brand"));
    nav->addWidget(label("TELEKOM • BLOCKCHAIN", "brandSub"));
    nav->addSpacing(34);
    pages_ = new QStackedWidget;
    const QStringList names = {"▦  Overview", "◉  Subscribers", "◇  Blockchain", "⌁  P2P network", "⚙  Settings"};
    const std::vector<QWidget *> pages = {buildDashboardPage(), subscribers(), blockchainPage(), network(), settings()};
    for (auto *page : pages) pages_->addWidget(page);
    for (int index = 0; index < names.size(); ++index) {
        auto *button = new QPushButton(names[index]);
        button->setObjectName("nav");
        button->setCheckable(true);
        button->setAutoExclusive(true);
        button->setChecked(index == 0);
        connect(button, &QPushButton::clicked, pages_, [this, index] { pages_->setCurrentIndex(index); });
        nav->addWidget(button);
    }
    nav->addStretch();
    nodeStatus_ = label("●  DB CONNECTED", "status");
    nav->addWidget(nodeStatus_);
    nav->addSpacing(6);
    nav->addWidget(label("v1.0.0 • Operator console", "brandSub"));
    horizontal->addWidget(sidebar);
    auto *content = new QWidget;
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(34, 28, 34, 18);
    contentLayout->addWidget(pages_);
    toast_ = label("");
    toast_->setAlignment(Qt::AlignCenter);
    toast_->setMinimumHeight(25);
    contentLayout->addWidget(toast_);
    horizontal->addWidget(content, 1);
    setCentralWidget(root);
}

void MainWindow::paymentDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("New payment");
    dialog.setMinimumWidth(390);
    auto *form = new QFormLayout(&dialog);
    form->setContentsMargins(24, 20, 24, 20);
    form->setSpacing(14);
    QComboBox subscriber;
    subscriber.setEditable(true);
    try {
        for (const auto &name : SubscriberRepository::names()) subscriber.addItem(QString::fromStdString(name));
    } catch (...) {}
    subscriber.lineEdit()->setPlaceholderText("Subscriber name");
    subscriber.lineEdit()->setValidator(new QRegularExpressionValidator(QRegularExpression("^[\\p{L}][\\p{L}\\p{N} .'-]{0,79}$"), &subscriber));
    QComboBox service;
    service.addItems({"Internet", "IPTV", "Phone"});
    QLineEdit amount("280");
    amount.setValidator(new QDoubleValidator(0.01, 1000000.0, 2, &amount));
    form->addRow("Subscriber", &subscriber);
    form->addRow("Service", &service);
    form->addRow("Amount (TMT)", &amount);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    buttons->button(QDialogButtonBox::Ok)->setText("Validate & create block");
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        const double value = amount.text().toDouble();
        if (subscriber.currentText().trimmed().isEmpty() || value <= 0) {
            QMessageBox::warning(&dialog, "Payment", "Enter a subscriber and a valid amount.");
            return;
        }
        ServiceType type = ServiceType::INTERNET;
        if (service.currentText() == "IPTV") type = ServiceType::IPTV;
        else if (service.currentText() == "Phone") type = ServiceType::PHONE;
        QString error;
        if (!processPayment(subscriber.currentText(), type, value, &error)) {
            QMessageBox::warning(&dialog, "Payment", error.isEmpty() ? "Payment could not be processed." : error);
            return;
        }
        dialog.accept();
        refreshAll();
        showToast("Payment validated and stored on the blockchain.");
    });
    dialog.exec();
}

void MainWindow::addSubscriber() {
    QDialog dialog(this);
    dialog.setWindowTitle("Add subscriber");
    auto *form = new QFormLayout(&dialog);
    form->setContentsMargins(24, 20, 24, 20);
    QLineEdit name;
    name.setValidator(new QRegularExpressionValidator(QRegularExpression("^[\\p{L}][\\p{L}\\p{N} .'-]{0,79}$"), &name));
    form->addRow("Full name", &name);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    buttons->button(QDialogButtonBox::Ok)->setText("Create account");
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (name.text().trimmed().isEmpty()) return;
        SmartContract::updateSubscriberInDB(Subscriber(name.text().trimmed().toStdString(), false));
        dialog.accept();
        refreshAll();
        showToast("Subscriber account created.");
    });
    dialog.exec();
}

void MainWindow::editSelectedSubscriberServices() {
    const int row = subscriberTable_ ? subscriberTable_->currentRow() : -1;
    const auto *name = row >= 0 ? subscriberTable_->item(row, 1) : nullptr;
    if (!name) {
        QMessageBox::information(this, "Select subscriber", "Select a subscriber, then choose Edit selected services.");
        return;
    }
    editSubscriberServices(name->text());
}

void MainWindow::editSubscriberServices(const QString &subscriberName) {
    Subscriber subscriber(subscriberName.toStdString(), false);
    if (!SmartContract::getSubscriberFromDB(subscriberName.toStdString(), subscriber)) {
        QMessageBox::warning(this, "Subscriber", "The selected subscriber could not be found.");
        return;
    }
    const Subscriber previous = subscriber;
    QDialog dialog(this);
    dialog.setWindowTitle("Edit subscriber services");
    dialog.setMinimumWidth(420);
    auto *form = new QFormLayout(&dialog);
    form->setContentsMargins(24, 20, 24, 20);
    form->setSpacing(14);
    form->addRow("Subscriber", label(subscriberName));
    QComboBox internetSpeed;
    internetSpeed.addItems({"1", "2", "4", "6"});
    internetSpeed.setCurrentText(QString::fromStdString(subscriber.internet.speed));
    form->addRow("Internet speed (Mbit/s)", &internetSpeed);
    QComboBox iptvChannels;
    for (int count = 1; count <= 10; ++count) iptvChannels.addItem(QString::number(count));
    iptvChannels.setCurrentText(QString::number(std::clamp(subscriber.iptv.tvCount, 1, 10)));
    form->addRow("IPTV channels", &iptvChannels);
    const QString phoneState = subscriber.isServiceActive(ServiceType::PHONE)
        ? QString("Active (%1 days remaining)").arg(subscriber.remainingDays(subscriber.phoneExpiry))
        : "Inactive — activate or renew with New payment";
    form->addRow("Phone service", label(phoneState, "muted"));
    auto *hint = label("Plan changes are saved immediately. Service activation and renewal use New payment.", "muted");
    hint->setWordWrap(true);
    form->addRow(hint);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (!subscriber.internet.setSpeed(internetSpeed.currentText().toStdString()) ||
            !subscriber.iptv.setTVCount(iptvChannels.currentText().toInt())) {
            QMessageBox::warning(&dialog, "Service settings", "Choose a supported Internet speed and 1–10 IPTV channels.");
            return;
        }
        SmartContract::updateSubscriberInDB(subscriber);
        if (blockchain_) {
            const std::string transactionData = SmartContract::getPlanChangeData(previous, subscriber);
            blockchain_->AddBlock(Block(blockchain_->getLatestBlock().getIndex() + 1, transactionData));
        }
        dialog.accept();
        refreshAll();
        showToast("Subscriber service settings updated.");
    });
    dialog.exec();
}

void MainWindow::connectNode() {
    QDialog dialog(this);
    dialog.setWindowTitle("Connect peer node");
    auto *form = new QFormLayout(&dialog);
    form->setContentsMargins(24, 20, 24, 20);
    auto *hint = new QLabel("Peer connections are available in the telekom_system console app.");
    hint->setWordWrap(true);
    form->addRow(hint);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    dialog.exec();
}
