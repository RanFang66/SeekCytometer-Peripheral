#include "ChipPositionDao.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

bool ChipPositionDao::insert(const ChipPosition &record)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("INSERT INTO chip_position (name, x_position, y_position) "
                  "VALUES (:name, :x, :y)");
    query.bindValue(":name", record.name);
    query.bindValue(":x", record.xPosition);
    query.bindValue(":y", record.yPosition);

    if (!query.exec()) {
        qWarning() << "ChipPositionDao::insert failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool ChipPositionDao::update(const ChipPosition &record)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("UPDATE chip_position SET name = :name, x_position = :x, "
                  "y_position = :y WHERE id = :id");
    query.bindValue(":name", record.name);
    query.bindValue(":x", record.xPosition);
    query.bindValue(":y", record.yPosition);
    query.bindValue(":id", record.id);

    if (!query.exec()) {
        qWarning() << "ChipPositionDao::update failed:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool ChipPositionDao::remove(int id)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("DELETE FROM chip_position WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "ChipPositionDao::remove failed:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

ChipPosition ChipPositionDao::queryById(int id)
{
    ChipPosition result;
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id, name, x_position, y_position FROM chip_position WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec() && query.next()) {
        result.id = query.value(0).toInt();
        result.name = query.value(1).toString();
        result.xPosition = query.value(2).toInt();
        result.yPosition = query.value(3).toInt();
    }
    return result;
}

QList<ChipPosition> ChipPositionDao::queryAll()
{
    QList<ChipPosition> list;
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec("SELECT id, name, x_position, y_position FROM chip_position ORDER BY id");

    while (query.next()) {
        ChipPosition r;
        r.id = query.value(0).toInt();
        r.name = query.value(1).toString();
        r.xPosition = query.value(2).toInt();
        r.yPosition = query.value(3).toInt();
        list.append(r);
    }
    return list;
}

QStringList ChipPositionDao::queryAllNames()
{
    QStringList names;
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec("SELECT name FROM chip_position ORDER BY id");

    while (query.next()) {
        names.append(query.value(0).toString());
    }
    return names;
}

ChipPosition ChipPositionDao::queryByName(const QString &name)
{
    ChipPosition result;
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id, name, x_position, y_position FROM chip_position WHERE name = :name");
    query.bindValue(":name", name);

    if (query.exec() && query.next()) {
        result.id = query.value(0).toInt();
        result.name = query.value(1).toString();
        result.xPosition = query.value(2).toInt();
        result.yPosition = query.value(3).toInt();
    }
    return result;
}
