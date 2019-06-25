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
class BaseModel
{
public:
    BaseModel();
    BaseModel(const QJsonObject& obj);

    void init(const QJsonObject& obj);

    // Read-Only Public Getters
    const optional<QString>& name();
    const optional<vars>& index();

private:
    /**
     * Stage 1 Variables: Extracted from JSON Data
     */

    // Name of data entry
    optional<QString> _name;

    // Internal game index of data entry
    optional<vars> _index;
};

Q_DECLARE_METATYPE(BaseModel)

#endif // BASEMODEL_H
