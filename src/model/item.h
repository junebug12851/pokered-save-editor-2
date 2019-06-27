#ifndef ITEM_H
#define ITEM_H

#include "basemodel.h"

class Move;
class Item;

using _ItemArr = vector<Item*>;
using _ItemDb = unordered_map<QString, Item*>;

using ItemArr = const _ItemArr*;
using ItemDb = const _ItemDb*;

class Item : public BaseModel
{
    Q_OBJECT
    Q_PROPERTY(const optional<bool>& glitch READ glitch CONSTANT FINAL)
    Q_PROPERTY(const optional<bool>& common READ common CONSTANT FINAL)
    Q_PROPERTY(const optional<vars>& tm READ tm CONSTANT FINAL)
    Q_PROPERTY(const optional<vars>& hm READ hm CONSTANT FINAL)
    Q_PROPERTY(const optional<Move*>& toTmMove READ toTmMove CONSTANT FINAL)
    Q_PROPERTY(const optional<Move*>& toHmMove READ toHmMove CONSTANT FINAL)
    Q_PROPERTY(ItemArr store READ store CONSTANT FINAL)
    Q_PROPERTY(ItemDb db READ db CONSTANT FINAL)

public:
    Item();
    Item(const QJsonObject& obj);

    void init(const QJsonObject& obj);

    const optional<bool>& glitch();
    const optional<bool>& common();
    const optional<vars>& tm();
    const optional<vars>& hm();

    const optional<Move*>& toTmMove();
    const optional<Move*>& toHmMove();

    /**
     * Data Store and other static properties and methods related to building
     * and maintaining the store
     */
    static ItemArr store();
    static ItemDb db();

    // Creates the JSON store from a JSON file
    static void initStore(const QString& filename);

    // Inits DB which indexes the store
    static void initDb();

    // This deep links the store to models in other stores
    static void initDeepLink();

private:
    /**
     * Stage 1 Variables: Extracted from JSON Data
     */

    // Is this a normal or a glitch item
    optional<bool> _glitch;

    // Is this a common item
    // The alternative would be a specially given item
    // Pokeballs and Fire Stones are common items (They can be accumulated)
    // Master Balls and Bikes are not common (Their can only be one)
    optional<bool> _common;

    // Does this represent a tm or hm and if so which one?
    // Note that hms internally are tms so the tm will also be present for hms
    // and the hm will reflect the real number
    optional<vars> _tm;
    optional<vars> _hm;

    /**
     * Stage 2 Variables: Inter-Linking amongst data
     */
    optional<Move*> _toTmMove;
    optional<Move*> _toHmMove;

    /**
     * Data Store and other static properties and methods related to building
     * and maintaining the store
     */
    static _ItemArr _store; // Sequential Items

    // Index
    // BaseModel does some of the initial indexing of it's own
    // tm + # (tm7)
    // hm + # (hm2)
    // tm + ## (tm07)
    // hm + ## (hm02)
    static _ItemDb _db; // Indexed for lookup
};

#endif // ITEM_H
