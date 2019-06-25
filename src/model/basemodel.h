#ifndef BASEMODEL_H
#define BASEMODEL_H

#include <QtCore/QObject>
#include <QString>
#include <QJsonObject>
#include <optional>

using std::optional;

#include "../includes/vars.h"

/**
  * Base model to all models
  * Most Every model contains these values
 */
struct BaseModel
{
    BaseModel();
    BaseModel(const QJsonObject& obj);

    // Name of data entry
    optional<QString> name;

    // Internal game index of data entry
    optional<vars> index;
};

Q_DECLARE_METATYPE(BaseModel)

#endif // BASEMODEL_H
