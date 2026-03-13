#ifndef LENSPOSITIONDAO_H
#define LENSPOSITIONDAO_H

#include "DatabaseModels.h"
#include <QList>
#include <QStringList>

class LensPositionDao
{
public:
    LensPositionDao() = default;

    bool insert(const LensPosition &record);
    bool update(const LensPosition &record);
    bool remove(int id);
    LensPosition queryById(int id);
    QList<LensPosition> queryAll();
    QStringList queryAllNames();
    LensPosition queryByName(const QString &name);
};

#endif // LENSPOSITIONDAO_H
