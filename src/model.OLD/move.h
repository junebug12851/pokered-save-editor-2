#ifndef MOVES_H
#define MOVES_H

#include "basemodel.h"

class Item;
class Move;
class Type;

using _MoveArr = vector<Move*>;
using _MoveDb = unordered_map<QString, Move*>;

using MoveArr = const _MoveArr*;
using MoveDb = const _MoveDb*;

class Move : public BaseModel
{
    Q_OBJECT
    Q_PROPERTY(const optional<vars>& power READ power CONSTANT FINAL)
    Q_PROPERTY(const optional<QString>& type READ type CONSTANT FINAL)
    Q_PROPERTY(const optional<vars>& accuracy READ accuracy CONSTANT FINAL)
    Q_PROPERTY(const optional<vars>& pp READ pp CONSTANT FINAL)
    Q_PROPERTY(const optional<vars>& tm READ tm CONSTANT FINAL)
    Q_PROPERTY(const optional<vars>& hm READ hm CONSTANT FINAL)
    Q_PROPERTY(const optional<bool>& glitch READ glitch CONSTANT FINAL)
    Q_PROPERTY(const optional<Item*>& toTmItem READ toTmItem CONSTANT FINAL)
    Q_PROPERTY(const optional<Item*>& toHmItem READ toHmItem CONSTANT FINAL)
    Q_PROPERTY(const optional<Type*>& toType READ toType CONSTANT FINAL)
    Q_PROPERTY(MoveArr store READ store CONSTANT FINAL)
    Q_PROPERTY(MoveDb db READ db CONSTANT FINAL)

public:
    Move();
    Move(const QJsonObject& obj);

    void init(const QJsonObject& obj);

    const optional<vars>& power();
    const optional<QString>& type();
    const optional<vars>& accuracy();
    const optional<vars>& pp();
    const optional<vars>& tm();
    const optional<vars>& hm();
    const optional<bool>& glitch();

    const optional<Item*>& toTmItem();
    const optional<Item*>& toHmItem();
    const optional<Type*>& toType();

    /**
     * Data Store and other static properties and methods related to building
     * and maintaining the store
     */
    static MoveArr store();
    static MoveDb db();
    static void initStore(const QString& filename);
    static void initDb();

    // This deep links the store to models in other stores
    static void initDeepLink();

private:
    /**
     * Stage 1 Variables: Extracted from JSON Data
     */

    // Move Power
    optional<vars> _power;

    // Move Type spelled out
    optional<QString> _type;

    // Move Accuracy as an interger percent (ex: 100 == 100%)
    optional<vars> _accuracy;

    // Move PP
    optional<vars> _pp;

    // Move internal TM index
    // Please note internally HM's are also TM's so this will be present on HM's
    // as well
    optional<vars> _tm;

    // Actual HM Number
    optional<vars> _hm;

    // Is this a glitch move?
    // Glitch moves are often highly incomplete meaning
    optional<bool> _glitch;

    /**
     * Stage 2 Variables: Inter-Linking amongst data
     */

    optional<Item*> _toTmItem;
    optional<Item*> _toHmItem;
    optional<Type*> _toType;

    /**
     * Data Store and other static properties and methods related to building
     * and maintaining the store
     */
    static _MoveArr _store;

    // Index
    // BaseModel does some of the initial indexing of it's own
    // tm + # (tm7)
    // hm + # (hm2)
    // tm + ## (tm07)
    // hm + ## (hm02)
    static _MoveDb _db; // Indexed for lookup
};

#endif // MOVES_H
