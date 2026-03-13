#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QString>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    static DatabaseManager &instance();

    bool connect(const QString &host = "localhost",
                 int port = 5432,
                 const QString &dbName = "seekcytometer_peripheral",
                 const QString &user = "postgres",
                 const QString &password = "kissfire");
    void disconnect();
    bool isConnected() const;

    QSqlDatabase database() const;

    bool initTables();

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &msg);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager() override;
    Q_DISABLE_COPY(DatabaseManager)

    QSqlDatabase m_db;
};

#endif // DATABASEMANAGER_H
