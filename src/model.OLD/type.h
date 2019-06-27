#ifndef TYPE_H
#define TYPE_H

#include "basemodel.h"

class Type;

using _TypeArr = vector<Type*>;
using _TypeDb = unordered_map<QString, Type*>;

using TypeArr = const _TypeArr*;
using TypeDb = const _TypeDb*;

class Type : public BaseModel
{
    Q_OBJECT

public:
    Type();
    Type(const QJsonObject& obj);

    // Type only has name and index, BaseModel suffices

    /**
     * Data Store and other static properties and methods related to building
     * and maintaining the store
     */
    static TypeArr store();
    static TypeDb db();
    static void initStore(const QString& filename);
    static void initDb();

private:
    /**
     * Data Store and other static properties and methods related to building
     * and maintaining the store
     */
    static _TypeArr _store;

    // Index
    // BaseModel does some of the initial indexing of it's own
    static _TypeDb _db; // Indexed for lookup
};

#endif // TYPE_H
