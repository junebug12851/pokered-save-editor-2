#ifndef BASEMODEL_H
#define BASEMODEL_H

#include <QtCore/QObject>
#include <QString>
#include <QJsonObject>

#include "../includes/vars.h"

/**
  * Base model to all models
  * Every model contains these values
 */
struct BaseModel
{
    // Name of data entry
    QString name;

    // Internal game index of data entry
    var index;

    static void fromJson(BaseModel& model, QJsonObject obj);
};

Q_DECLARE_METATYPE(BaseModel)

#endif // BASEMODEL_H
