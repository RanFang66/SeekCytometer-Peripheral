#include "HydraulicControlParamsDao.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

static const char *kSelectAll =
    "SELECT id, name, "
    "ch1_kp, ch1_ki, ch1_feedforward, "
    "ch2_kp, ch2_ki, ch2_feedforward, "
    "ch3_kp, ch3_ki, ch3_feedforward, "
    "ch4_kp, ch4_ki, ch4_feedforward, "
    "ch5_kp, ch5_ki, ch5_feedforward "
    "FROM hydraulic_control_params";

static HydraulicControlParams readRecord(QSqlQuery &query)
{
    HydraulicControlParams r;
    int i = 0;
    r.id   = query.value(i++).toInt();
    r.name = query.value(i++).toString();
    r.ch1Kp = query.value(i++).toFloat();
    r.ch1Ki = query.value(i++).toFloat();
    r.ch1Feedforward = query.value(i++).toInt();
    r.ch2Kp = query.value(i++).toFloat();
    r.ch2Ki = query.value(i++).toFloat();
    r.ch2Feedforward = query.value(i++).toInt();
    r.ch3Kp = query.value(i++).toFloat();
    r.ch3Ki = query.value(i++).toFloat();
    r.ch3Feedforward = query.value(i++).toInt();
    r.ch4Kp = query.value(i++).toFloat();
    r.ch4Ki = query.value(i++).toFloat();
    r.ch4Feedforward = query.value(i++).toInt();
    r.ch5Kp = query.value(i++).toFloat();
    r.ch5Ki = query.value(i++).toFloat();
    r.ch5Feedforward = query.value(i++).toInt();
    return r;
}

bool HydraulicControlParamsDao::insert(const HydraulicControlParams &record)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(
        "INSERT INTO hydraulic_control_params "
        "(name, ch1_kp, ch1_ki, ch1_feedforward, "
        "ch2_kp, ch2_ki, ch2_feedforward, "
        "ch3_kp, ch3_ki, ch3_feedforward, "
        "ch4_kp, ch4_ki, ch4_feedforward, "
        "ch5_kp, ch5_ki, ch5_feedforward) "
        "VALUES (:name, :c1kp, :c1ki, :c1ff, "
        ":c2kp, :c2ki, :c2ff, "
        ":c3kp, :c3ki, :c3ff, "
        ":c4kp, :c4ki, :c4ff, "
        ":c5kp, :c5ki, :c5ff)");
    query.bindValue(":name", record.name);
    query.bindValue(":c1kp", record.ch1Kp);
    query.bindValue(":c1ki", record.ch1Ki);
    query.bindValue(":c1ff", record.ch1Feedforward);
    query.bindValue(":c2kp", record.ch2Kp);
    query.bindValue(":c2ki", record.ch2Ki);
    query.bindValue(":c2ff", record.ch2Feedforward);
    query.bindValue(":c3kp", record.ch3Kp);
    query.bindValue(":c3ki", record.ch3Ki);
    query.bindValue(":c3ff", record.ch3Feedforward);
    query.bindValue(":c4kp", record.ch4Kp);
    query.bindValue(":c4ki", record.ch4Ki);
    query.bindValue(":c4ff", record.ch4Feedforward);
    query.bindValue(":c5kp", record.ch5Kp);
    query.bindValue(":c5ki", record.ch5Ki);
    query.bindValue(":c5ff", record.ch5Feedforward);

    if (!query.exec()) {
        qWarning() << "HydraulicControlParamsDao::insert failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool HydraulicControlParamsDao::update(const HydraulicControlParams &record)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(
        "UPDATE hydraulic_control_params SET name = :name, "
        "ch1_kp = :c1kp, ch1_ki = :c1ki, ch1_feedforward = :c1ff, "
        "ch2_kp = :c2kp, ch2_ki = :c2ki, ch2_feedforward = :c2ff, "
        "ch3_kp = :c3kp, ch3_ki = :c3ki, ch3_feedforward = :c3ff, "
        "ch4_kp = :c4kp, ch4_ki = :c4ki, ch4_feedforward = :c4ff, "
        "ch5_kp = :c5kp, ch5_ki = :c5ki, ch5_feedforward = :c5ff "
        "WHERE id = :id");
    query.bindValue(":name", record.name);
    query.bindValue(":c1kp", record.ch1Kp);
    query.bindValue(":c1ki", record.ch1Ki);
    query.bindValue(":c1ff", record.ch1Feedforward);
    query.bindValue(":c2kp", record.ch2Kp);
    query.bindValue(":c2ki", record.ch2Ki);
    query.bindValue(":c2ff", record.ch2Feedforward);
    query.bindValue(":c3kp", record.ch3Kp);
    query.bindValue(":c3ki", record.ch3Ki);
    query.bindValue(":c3ff", record.ch3Feedforward);
    query.bindValue(":c4kp", record.ch4Kp);
    query.bindValue(":c4ki", record.ch4Ki);
    query.bindValue(":c4ff", record.ch4Feedforward);
    query.bindValue(":c5kp", record.ch5Kp);
    query.bindValue(":c5ki", record.ch5Ki);
    query.bindValue(":c5ff", record.ch5Feedforward);
    query.bindValue(":id", record.id);

    if (!query.exec()) {
        qWarning() << "HydraulicControlParamsDao::update failed:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool HydraulicControlParamsDao::remove(int id)
{
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("DELETE FROM hydraulic_control_params WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "HydraulicControlParamsDao::remove failed:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

HydraulicControlParams HydraulicControlParamsDao::queryById(int id)
{
    HydraulicControlParams result;
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QString("%1 WHERE id = :id").arg(kSelectAll));
    query.bindValue(":id", id);

    if (query.exec() && query.next())
        result = readRecord(query);
    return result;
}

QList<HydraulicControlParams> HydraulicControlParamsDao::queryAll()
{
    QList<HydraulicControlParams> list;
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec(QString("%1 ORDER BY id").arg(kSelectAll));

    while (query.next())
        list.append(readRecord(query));
    return list;
}

QStringList HydraulicControlParamsDao::queryAllNames()
{
    QStringList names;
    QSqlQuery query(DatabaseManager::instance().database());
    query.exec("SELECT name FROM hydraulic_control_params ORDER BY id");

    while (query.next())
        names.append(query.value(0).toString());
    return names;
}

HydraulicControlParams HydraulicControlParamsDao::queryByName(const QString &name)
{
    HydraulicControlParams result;
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(QString("%1 WHERE name = :name").arg(kSelectAll));
    query.bindValue(":name", name);

    if (query.exec() && query.next())
        result = readRecord(query);
    return result;
}
