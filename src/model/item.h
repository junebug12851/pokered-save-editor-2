#ifndef ITEM_H
#define ITEM_H

#include "basemodel.h"

struct Item : public BaseModel
{
    Item();
    Item(const QJsonObject& obj);

    // Is this a normal or a glitch item
    optional<bool> normal;

    // Is this a common item
    // The alternative would be a specially given item
    // Pokeballs and Fire Stones are common items (They can be accumulated)
    // Master Balls and Bikes are not common (Their can only be one)
    optional<bool> common;
};

Q_DECLARE_METATYPE(Item)

#endif // ITEM_H
