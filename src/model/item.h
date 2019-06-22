#ifndef ITEM_H
#define ITEM_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>
#include <QVector>

#include "basemodel.h"

class Item : public BaseModel
{
    Q_OBJECT
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

    static const QVector<Item> store;
};

#endif // ITEM_H
