#ifndef ITEM_H
#define ITEM_H

#include "basemodel.h"

class Item : public BaseModel
{
public:
    Item();
    Item(const QJsonObject& obj);

    void init(const QJsonObject& obj);

    const optional<bool>& normal();
    const optional<bool>& common();

private:
    // Is this a normal or a glitch item
    optional<bool> _normal;

    // Is this a common item
    // The alternative would be a specially given item
    // Pokeballs and Fire Stones are common items (They can be accumulated)
    // Master Balls and Bikes are not common (Their can only be one)
    optional<bool> _common;
};

Q_DECLARE_METATYPE(Item)

#endif // ITEM_H
