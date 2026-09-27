#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <ctime>
#include <optional>
#include <string>

class QLabel;
class QTableWidget;
struct Subscriber;
struct Block;

QString applicationStyle();
QLabel *label(const QString &text, const QString &name = {});
QTableWidget *table(const QStringList &headers);
void setTableRows(QTableWidget *target, const QList<QStringList> &rows);

QString truncateHash(const std::string &hash);
QString formatBlockTime(std::time_t timestamp);
QString subscriberStatusLabel(const Subscriber &subscriber);
QString internetLabel(const Subscriber &subscriber);
QString iptvLabel(const Subscriber &subscriber);
QString phoneLabel(const Subscriber &subscriber);
QString blockSummary(const Block &block);
std::optional<std::string> fieldFromBlockData(const std::string &data, const std::string &key);
std::optional<double> paymentFromBlockData(const std::string &data);
