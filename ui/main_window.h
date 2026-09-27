#pragma once

#include <QMainWindow>

#include <memory>

class Blockchain;
class QLabel;
class QStackedWidget;
class QTableWidget;
class QWidget;
class QString;
enum class ServiceType;

class MainWindow final : public QMainWindow {
public:
    MainWindow();
    ~MainWindow() override;

private:
    std::unique_ptr<Blockchain> blockchain_;
    QStackedWidget *pages_ = nullptr;
    QTableWidget *blockTable_ = nullptr;
    QTableWidget *subscriberTable_ = nullptr;
    QTableWidget *recentTable_ = nullptr;
    QLabel *blockCount_ = nullptr;
    QLabel *toast_ = nullptr;
    QLabel *nodeStatus_ = nullptr;
    QLabel *subscriberMetric_ = nullptr;
    QLabel *blockMetric_ = nullptr;
    QLabel *paymentsMetric_ = nullptr;
    QLabel *paymentsDetail_ = nullptr;

    QWidget *cardWithValue(const QString &value, const QString &caption, const QString &detail, QLabel **valueOut);
    QWidget *pageHeader(const QString &title, const QString &subtitle, QWidget *right = nullptr);
    QWidget *buildDashboardPage();
    QWidget *subscribers();
    QWidget *blockchainPage();
    QWidget *network();
    QWidget *settings();

    void build();
    void refreshAll();
    void refreshDashboard();
    void refreshSubscribers();
    void refreshBlocks();
    int countSubscribers();

    bool processPayment(const QString &subscriberName, ServiceType service, double amount, QString *errorOut);
    void paymentDialog();
    void addSubscriber();
    void editSelectedSubscriberServices();
    void editSubscriberServices(const QString &subscriberName);
    void runAudit();
    void connectNode();
    void showToast(const QString &message, bool error = false);
};
