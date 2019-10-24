#ifndef ITEM_H
#define ITEM_H

#include "basemodel.h"

class Move;

class Item : public BaseModel<Item>
{
public:
    enum keys: var8 {
        // Continue where the parent left off
        key_glitch = BaseModel<Item>::keystore_size,
        key_common,
        key_tm,
        key_hm,
        key_to_tm_move,
        key_to_hm_move,
        keystore_size
    };

    Item();
    Item(QJsonObject& obj);

    optional<bool*> glitch();
    optional<bool*> common();
    optional<var8*> tm();
    optional<var8*> hm();
    optional<Move*> toTmMove();
    optional<Move*> toHmMove();

    // Indexes a store to a db for speedy lookup
    static void initDb();

    // Deep link items from this store with items in other stores
    static void initDeepLink();
private:
    // Init Model
    void init(QJsonObject& obj);
};

#endif // ITEM_H
