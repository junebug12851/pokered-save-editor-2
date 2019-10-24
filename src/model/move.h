#ifndef MOVE_H
#define MOVE_H

#include "basemodel.h"

class Item;
class Type;

class Move : public BaseModel<Move>
{
public:
    enum keys: var8 {
        // Continue where the parent left off
        key_power = BaseModel<Move>::keystore_size,
        key_type,
        key_accuracy,
        key_pp,
        key_tm,
        key_hm,
        key_glitch,
        key_to_tm_item,
        key_to_hm_item,
        key_to_type,
        keystore_size
    };

    Move();
    Move(QJsonObject& obj);

    optional<var8*> power();
    optional<QString*> type();
    optional<var8*> accuracy();
    optional<var8*> pp();
    optional<var8*> tm();
    optional<var8*> hm();
    optional<bool*> glitch();

    optional<Item*> toTmItem();
    optional<Item*> toHmItem();
    optional<Type*> toType();

    // Indexes a store to a db for speedy lookup
    static void initDb();

    // Deep link items from this store with items in other stores
    static void initDeepLink();
private:
    // Init Model
    void init(QJsonObject& obj);
};

#endif // MOVE_H
