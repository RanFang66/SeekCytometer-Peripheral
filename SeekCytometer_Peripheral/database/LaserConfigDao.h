#ifndef LASERCONFIGDAO_H
#define LASERCONFIGDAO_H

#include "DatabaseModels.h"
#include <QList>
#include <QStringList>

class LaserConfigDao
{
public:
    LaserConfigDao() = default;

    bool insert(const LaserConfig &record);
    bool update(const LaserConfig &record);
    bool remove(int id);
    LaserConfig queryById(int id);
    QList<LaserConfig> queryAll();
    QStringList queryAllNames();
    LaserConfig queryByName(const QString &name);
};

#endif // LASERCONFIGDAO_H
