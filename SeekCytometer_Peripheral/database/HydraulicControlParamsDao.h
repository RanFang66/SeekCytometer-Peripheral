#ifndef HYDRAULICCONTROLPARAMSDAO_H
#define HYDRAULICCONTROLPARAMSDAO_H

#include "DatabaseModels.h"
#include <QList>
#include <QStringList>

class HydraulicControlParamsDao
{
public:
    HydraulicControlParamsDao() = default;

    bool insert(const HydraulicControlParams &record);
    bool update(const HydraulicControlParams &record);
    bool remove(int id);
    HydraulicControlParams queryById(int id);
    QList<HydraulicControlParams> queryAll();
    QStringList queryAllNames();
    HydraulicControlParams queryByName(const QString &name);
};

#endif // HYDRAULICCONTROLPARAMSDAO_H
