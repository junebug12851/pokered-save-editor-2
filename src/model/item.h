#ifndef ITEM_H
#define ITEM_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

#include "basemodel.h"

class Item : public BaseModel
{
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER name CONSTANT FINAL)
    Q_PROPERTY(quint8 index MEMBER index CONSTANT FINAL)
    Q_PROPERTY(bool normal MEMBER normal CONSTANT FINAL)
    Q_PROPERTY(bool typical MEMBER typical CONSTANT FINAL)

public:
    explicit Item(
            const QString name,
            const quint8 index,
            const bool normal,
            const bool typical,
            QObject *parent = nullptr);

    // Is this a normal or a glitch item
    const bool normal;

    // Is this an typical item
    // The alternative would be a specially given item
    const bool typical;
};

#endif // ITEM_H
