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

private:
    // Apparently another Qt gotcha is you need to use Q_DECLARE_METATYPE so
    // that it'll work with Qt types and after doing so can't use custom types
    // with qt because it's a custom type making me wonder what the point was
    // entirely
    optional<Item*> _toTmItem;
    optional<Item*> _toHmItem;
    optional<Type*> _toType;

    // Init Model
    void init(QJsonObject& obj);

    // Indexes a store to a db for speedy lookup
    static void initDb();

    // Deep link items from this store with items in other stores
    static void initDeepLink();
};

Q_DECLARE_METATYPE(Move)
Q_DECLARE_METATYPE(Move*)

#endif // MOVE_H
