#ifndef ITEM_H
#define ITEM_H

#include "basemodel.h"

class Move;

class Item : public BaseModel
{
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
    static const vector<Item*>& store();
    static const unordered_map<QString, Item*>& db();

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
    static vector<Item*> _store; // Sequential Items

    // Index
    // BaseModel does some of the initial indexing of it's own
    // tm + # (tm7)
    // hm + # (hm2)
    // tm + ## (tm07)
    // hm + ## (hm02)
    static unordered_map<QString, Item*> _db; // Indexed for lookup
};

Q_DECLARE_METATYPE(Item)

#endif // ITEM_H
