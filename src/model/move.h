#ifndef MOVE_H
#define MOVE_H

#include "basemodel.h"

class Item;
class Type;

class Move : public BaseModel<Move>
{
public:
    enum keys: var {
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

    const optional<vars> power();
    const optional<QString> type();
    const optional<vars> accuracy();
    const optional<vars> pp();
    const optional<vars> tm();
    const optional<vars> hm();
    const optional<bool> glitch();

    const optional<Item*> toTmItem();
    const optional<Item*> toHmItem();
    const optional<Type*> toType();

    // Indexes a store to a db for speedy lookup
    static void initDb();

    // Deep link items from this store with items in other stores
    static void initDeepLink();
private:
    // Init Model
    void init(QJsonObject& obj);
};

Q_DECLARE_METATYPE(Move)
Q_DECLARE_METATYPE(Move*)

#endif // MOVE_H
