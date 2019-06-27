#ifndef MOVES_H
#define MOVES_H

#include "basemodel.h"

class Item;

class Move : public BaseModel
{
    Q_OBJECT

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

    /**
     * Data Store and other static properties and methods related to building
     * and maintaining the store
     */
    static const vector<Move*>& store();
    static const unordered_map<QString, Move*>& db();
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

    /**
     * Data Store and other static properties and methods related to building
     * and maintaining the store
     */
    static vector<Move*> _store;

    // Index
    // BaseModel does some of the initial indexing of it's own
    // tm + # (tm7)
    // hm + # (hm2)
    // tm + ## (tm07)
    // hm + ## (hm02)
    static unordered_map<QString, Move*> _db; // Indexed for lookup
};

#endif // MOVES_H
