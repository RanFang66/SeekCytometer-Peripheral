#include "LaserConfigDao.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

bool LaserConfigDao::insert(const LaserConfig &record)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("INSERT INTO laser_config (name, laser_638nm_enable, laser_448nm_enable, "
                  "white_led_enable, laser_638nm_power, laser_448nm_power, white_led_power) "
                  "VALUES (:name, :e638, :e448, :eled, :p638, :p448, :pled)");
    query.bindValue(":name", record.name);
    query.bindValue(":e638", record.laser638nmEnable);
    query.bindValue(":e448", record.laser448nmEnable);
    query.bindValue(":eled", record.whiteLedEnable);
    query.bindValue(":p638", record.laser638nmPower);
    query.bindValue(":p448", record.laser448nmPower);
    query.bindValue(":pled", record.whiteLedPower);

    if (!query.exec()) {
        qWarning() << "LaserConfigDao::insert failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool LaserConfigDao::update(const LaserConfig &record)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE laser_config SET name = :name, "
                  "laser_638nm_enable = :e638, laser_448nm_enable = :e448, "
                  "white_led_enable = :eled, laser_638nm_power = :p638, "
                  "laser_448nm_power = :p448, white_led_power = :pled "
                  "WHERE id = :id");
    query.bindValue(":name", record.name);
    query.bindValue(":e638", record.laser638nmEnable);
    query.bindValue(":e448", record.laser448nmEnable);
    query.bindValue(":eled", record.whiteLedEnable);
    query.bindValue(":p638", record.laser638nmPower);
    query.bindValue(":p448", record.laser448nmPower);
    query.bindValue(":pled", record.whiteLedPower);
    query.bindValue(":id", record.id);

    if (!query.exec()) {
        qWarning() << "LaserConfigDao::update failed:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool LaserConfigDao::remove(int id)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("DELETE FROM laser_config WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "LaserConfigDao::remove failed:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

static LaserConfig readRecord(QSqlQuery &query)
{
    LaserConfig r;
    r.id = query.value(0).toInt();
    r.name = query.value(1).toString();
    r.laser638nmEnable = query.value(2).toBool();
    r.laser448nmEnable = query.value(3).toBool();
    r.whiteLedEnable = query.value(4).toBool();
    r.laser638nmPower = query.value(5).toInt();
    r.laser448nmPower = query.value(6).toInt();
    r.whiteLedPower = query.value(7).toInt();
    return r;
}

static const char *kSelectAll =
    "SELECT id, name, laser_638nm_enable, laser_448nm_enable, "
    "white_led_enable, laser_638nm_power, laser_448nm_power, white_led_power "
    "FROM laser_config";

LaserConfig LaserConfigDao::queryById(int id)
{
    LaserConfig result;
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QString("%1 WHERE id = :id").arg(kSelectAll));
    query.bindValue(":id", id);

    if (query.exec() && query.next())
        result = readRecord(query);
    return result;
}

QList<LaserConfig> LaserConfigDao::queryAll()
{
    QList<LaserConfig> list;
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec(QString("%1 ORDER BY id").arg(kSelectAll));

    while (query.next())
        list.append(readRecord(query));
    return list;
}

QStringList LaserConfigDao::queryAllNames()
{
    QStringList names;
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec("SELECT name FROM laser_config ORDER BY id");

    while (query.next())
        names.append(query.value(0).toString());
    return names;
}

LaserConfig LaserConfigDao::queryByName(const QString &name)
{
    LaserConfig result;
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QString("%1 WHERE name = :name").arg(kSelectAll));
    query.bindValue(":name", name);

    if (query.exec() && query.next())
        result = readRecord(query);
    return result;
}
