#ifndef ITEM_H
#define ITEM_H

#include <QMetaType>
#include "basemodel.h"

class Move;

class Item : public BaseModel<Item>
{
public:
    enum keys: var {
        // Continue where the parent left off
        key_glitch = BaseModel<Item>::keystore_size,
        key_common,
        key_tm,
        key_hm,
        keystore_size
    };

    Item();
    Item(QJsonObject& obj);

    const optional<bool> glitch();
    const optional<bool> common();
    const optional<vars> tm();
    const optional<vars> hm();
    const optional<Move*> toTmMove();
    const optional<Move*> toHmMove();

private:
    // Apparently another Qt gotcha is you need to use Q_DECLARE_METATYPE so
    // that it'll work with Qt types and after doing so can't use custom types
    // with qt because it's a custom type making me wonder what the point was
    // entirely
    optional<Move*> _toTmMove;
    optional<Move*> _toHmMove;

    // Init Model
    void init(QJsonObject& obj);

    // Indexes a store to a db for speedy lookup
    static void initDb();

    // Deep link items from this store with items in other stores
    static void initDeepLink();
};

Q_DECLARE_METATYPE(Item)
Q_DECLARE_METATYPE(Item*)

#endif // ITEM_H
