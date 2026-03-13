#include "LensPositionDao.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

bool LensPositionDao::insert(const LensPosition &record)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("INSERT INTO lens_position (name, z_position) VALUES (:name, :z)");
    query.bindValue(":name", record.name);
    query.bindValue(":z", record.zPosition);

    if (!query.exec()) {
        qWarning() << "LensPositionDao::insert failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool LensPositionDao::update(const LensPosition &record)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE lens_position SET name = :name, z_position = :z WHERE id = :id");
    query.bindValue(":name", record.name);
    query.bindValue(":z", record.zPosition);
    query.bindValue(":id", record.id);

    if (!query.exec()) {
        qWarning() << "LensPositionDao::update failed:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool LensPositionDao::remove(int id)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("DELETE FROM lens_position WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "LensPositionDao::remove failed:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

LensPosition LensPositionDao::queryById(int id)
{
    LensPosition result;
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id, name, z_position FROM lens_position WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec() && query.next()) {
        result.id = query.value(0).toInt();
        result.name = query.value(1).toString();
        result.zPosition = query.value(2).toInt();
    }
    return result;
}

QList<LensPosition> LensPositionDao::queryAll()
{
    QList<LensPosition> list;
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec("SELECT id, name, z_position FROM lens_position ORDER BY id");

    while (query.next()) {
        LensPosition r;
        r.id = query.value(0).toInt();
        r.name = query.value(1).toString();
        r.zPosition = query.value(2).toInt();
        list.append(r);
    }
    return list;
}

QStringList LensPositionDao::queryAllNames()
{
    QStringList names;
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec("SELECT name FROM lens_position ORDER BY id");

    while (query.next()) {
        names.append(query.value(0).toString());
    }
    return names;
}

LensPosition LensPositionDao::queryByName(const QString &name)
{
    LensPosition result;
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id, name, z_position FROM lens_position WHERE name = :name");
    query.bindValue(":name", name);

    if (query.exec() && query.next()) {
        result.id = query.value(0).toInt();
        result.name = query.value(1).toString();
        result.zPosition = query.value(2).toInt();
    }
    return result;
}
