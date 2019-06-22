#ifndef BASEMODEL_H
#define BASEMODEL_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

/**
  * Base model to all model classes
  * Every model contains these values
 */

/**
 * Use Regex to generate Q_PROPERTY quickly and accurately
 * Regex:   ".*?const (.*?) (.*?),.*?"
 * Replace: "Q_PROPERTY($1 $2 MEMBER $2 CONSTANT FINAL)"
 * https://regex101.com
 */

class BaseModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER name CONSTANT FINAL)
    Q_PROPERTY(int index MEMBER index CONSTANT FINAL)

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
