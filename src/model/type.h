#ifndef TYPE_H
#define TYPE_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

#include "basemodel.h"

class Type : public BaseModel
{
    Q_OBJECT
public:
    explicit Type(
            const QString name,
            const quint8 index,
            QObject *parent = nullptr);

    // Type only has name and index, BaseModel suffices
};

#endif // TYPE_H
