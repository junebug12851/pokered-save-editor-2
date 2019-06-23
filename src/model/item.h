#ifndef ITEM_H
#define ITEM_H

#include "basemodel.h"

struct Item : public BaseModel
{
    // Is this a normal or a glitch item
    bool normal;

    // Is this an typical item
    // The alternative would be a specially given item
    bool typical;

    static Item fromJson(QJsonObject obj);
};

Q_DECLARE_METATYPE(Item)

#endif // ITEM_H
