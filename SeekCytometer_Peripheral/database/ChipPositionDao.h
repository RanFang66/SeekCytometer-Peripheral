#ifndef CHIPPOSITIONDAO_H
#define CHIPPOSITIONDAO_H

#include "DatabaseModels.h"
#include <QList>
#include <QStringList>

class ChipPositionDao
{
public:
    ChipPositionDao() = default;

    bool insert(const ChipPosition &record);
    bool update(const ChipPosition &record);
    bool remove(int id);
    ChipPosition queryById(int id);
    QList<ChipPosition> queryAll();
    QStringList queryAllNames();
    ChipPosition queryByName(const QString &name);
};

#endif // CHIPPOSITIONDAO_H
