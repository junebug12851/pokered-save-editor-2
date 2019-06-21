#ifndef BASEMODEL_H
#define BASEMODEL_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

/**
  * Base model to all model classes
  * Every model contains these values
 */

class BaseModel : public QObject
{
    Q_OBJECT
public:
    explicit BaseModel(
            const QString name,
            const int index,
            QObject *parent = nullptr
        );

    // Name of data entry
    const QString name;

    // Internal game index of data entry
    const int index;
};

#endif // BASEMODEL_H
