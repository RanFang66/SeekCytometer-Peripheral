#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

DatabaseManager &DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    disconnect();
}

bool DatabaseManager::connect(const QString &host, int port,
                              const QString &dbName,
                              const QString &user, const QString &password)
{
    if (m_db.isOpen())
        return true;

    m_db = QSqlDatabase::addDatabase("QPSQL", "seekcytometer");
    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setDatabaseName(dbName);
    m_db.setUserName(user);
    m_db.setPassword(password);

    if (!m_db.open()) {
        QString err = m_db.lastError().text();
        qWarning() << "Database connection failed:" << err;
        emit errorOccurred(err);
        return false;
    }

    qInfo() << "Database connected:" << dbName << "@" << host << ":" << port;
    emit connected();
    return true;
}

void DatabaseManager::disconnect()
{
    if (m_db.isOpen()) {
        m_db.close();
        emit disconnected();
    }
}

bool DatabaseManager::isConnected() const
{
    return m_db.isOpen();
}

QSqlDatabase DatabaseManager::database() const
{
    return m_db;
}

bool DatabaseManager::initTables()
{
    if (!m_db.isOpen())
        return false;

    QSqlQuery query(m_db);
    QStringList statements;

    statements << QStringLiteral(
        "CREATE TABLE IF NOT EXISTS chip_position ("
        "  id          SERIAL PRIMARY KEY,"
        "  name        VARCHAR(64) NOT NULL UNIQUE,"
        "  x_position  INTEGER NOT NULL CHECK (x_position BETWEEN -100000 AND 80000),"
        "  y_position  INTEGER NOT NULL CHECK (y_position BETWEEN -50000 AND 700000)"
        ")");

    statements << QStringLiteral(
        "CREATE TABLE IF NOT EXISTS lens_position ("
        "  id          SERIAL PRIMARY KEY,"
        "  name        VARCHAR(64) NOT NULL UNIQUE,"
        "  z_position  INTEGER NOT NULL CHECK (z_position BETWEEN -10000 AND 35000)"
        ")");

    statements << QStringLiteral(
        "CREATE TABLE IF NOT EXISTS laser_config ("
        "  id                  SERIAL PRIMARY KEY,"
        "  name                VARCHAR(64) NOT NULL UNIQUE,"
        "  laser_638nm_enable  BOOLEAN NOT NULL DEFAULT FALSE,"
        "  laser_448nm_enable  BOOLEAN NOT NULL DEFAULT FALSE,"
        "  white_led_enable    BOOLEAN NOT NULL DEFAULT FALSE,"
        "  laser_638nm_power   INTEGER NOT NULL DEFAULT 0 CHECK (laser_638nm_power BETWEEN 0 AND 100),"
        "  laser_448nm_power   INTEGER NOT NULL DEFAULT 0 CHECK (laser_448nm_power BETWEEN 0 AND 100),"
        "  white_led_power     INTEGER NOT NULL DEFAULT 0 CHECK (white_led_power BETWEEN 0 AND 100)"
        ")");

    statements << QStringLiteral(
        "CREATE TABLE IF NOT EXISTS hydraulic_control_params ("
        "  id                  SERIAL PRIMARY KEY,"
        "  name                VARCHAR(64) NOT NULL UNIQUE,"
        "  ch1_kp              REAL NOT NULL DEFAULT 2.0,"
        "  ch1_ki              REAL NOT NULL DEFAULT 1.0,"
        "  ch1_feedforward     INTEGER NOT NULL DEFAULT 13000,"
        "  ch2_kp              REAL NOT NULL DEFAULT 2.0,"
        "  ch2_ki              REAL NOT NULL DEFAULT 1.0,"
        "  ch2_feedforward     INTEGER NOT NULL DEFAULT 13000,"
        "  ch3_kp              REAL NOT NULL DEFAULT 2.0,"
        "  ch3_ki              REAL NOT NULL DEFAULT 1.0,"
        "  ch3_feedforward     INTEGER NOT NULL DEFAULT 13000,"
        "  ch4_kp              REAL NOT NULL DEFAULT 2.0,"
        "  ch4_ki              REAL NOT NULL DEFAULT 1.0,"
        "  ch4_feedforward     INTEGER NOT NULL DEFAULT 13000,"
        "  ch5_kp              REAL NOT NULL DEFAULT 2.0,"
        "  ch5_ki              REAL NOT NULL DEFAULT 1.0,"
        "  ch5_feedforward     INTEGER NOT NULL DEFAULT 13000"
        ")");

    for (const QString &sql : statements) {
        if (!query.exec(sql)) {
            QString err = query.lastError().text();
            qWarning() << "Failed to create table:" << err;
            emit errorOccurred(err);
            return false;
        }
    }

    // Insert default records (ON CONFLICT DO NOTHING avoids duplicates)
    QStringList inserts;

    inserts << QStringLiteral(
        "INSERT INTO chip_position (name, x_position, y_position) VALUES"
        "  ('默认位置', 0, 0),"
        "  ('检测区域', 10000, 350000),"
        "  ('进样口',   -5000, 5000)"
        " ON CONFLICT (name) DO NOTHING");

    inserts << QStringLiteral(
        "INSERT INTO lens_position (name, z_position) VALUES"
        "  ('默认位置', 0),"
        "  ('聚焦位置', 15000)"
        " ON CONFLICT (name) DO NOTHING");

    inserts << QStringLiteral(
        "INSERT INTO laser_config (name, laser_638nm_enable, laser_448nm_enable,"
        "  white_led_enable, laser_638nm_power, laser_448nm_power, white_led_power) VALUES"
        "  ('全部关闭',  FALSE, FALSE, FALSE, 0,  0,  0),"
        "  ('638nm激光', TRUE,  FALSE, FALSE, 50, 0,  0),"
        "  ('448nm激光', FALSE, TRUE,  FALSE, 0,  50, 0),"
        "  ('双激光',    TRUE,  TRUE,  FALSE, 50, 50, 0),"
        "  ('白光照明',  FALSE, FALSE, TRUE,  0,  0,  60)"
        " ON CONFLICT (name) DO NOTHING");

    inserts << QStringLiteral(
        "INSERT INTO hydraulic_control_params (name,"
        "  ch1_kp, ch1_ki, ch1_feedforward,"
        "  ch2_kp, ch2_ki, ch2_feedforward,"
        "  ch3_kp, ch3_ki, ch3_feedforward,"
        "  ch4_kp, ch4_ki, ch4_feedforward,"
        "  ch5_kp, ch5_ki, ch5_feedforward) VALUES"
        "  ('默认参数', 2.0,1.0,13000, 2.0,1.0,13000, 2.0,1.0,13000, 2.0,1.0,13000, 2.0,1.0,13000),"
        "  ('高压模式', 3.0,1.5,18000, 3.0,1.5,18000, 3.0,1.5,18000, 3.0,1.5,18000, 3.0,1.5,18000),"
        "  ('低压模式', 1.5,0.8,8000,  1.5,0.8,8000,  1.5,0.8,8000,  1.5,0.8,8000,  1.5,0.8,8000)"
        " ON CONFLICT (name) DO NOTHING");

    for (const QString &sql : inserts) {
        if (!query.exec(sql)) {
            qWarning() << "Failed to insert default data:" << query.lastError().text();
        }
    }

    qInfo() << "Database tables initialized successfully";
    return true;
}
