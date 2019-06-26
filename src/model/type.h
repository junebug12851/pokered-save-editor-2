#ifndef TYPE_H
#define TYPE_H

#include "basemodel.h"

class Type : public BaseModel
{
public:
    Type();
    Type(const QJsonObject& obj);

    // Type only has name and index, BaseModel suffices

    /**
     * Data Store and other static properties and methods related to building
     * and maintaining the store
     */
    static const vector<Type*>& store();
    static const unordered_map<QString, Type*>& db();
    static void initStore(const QString& filename);
    static void initDb();

private:
    /**
     * Data Store and other static properties and methods related to building
     * and maintaining the store
     */
    static vector<Type*> _store;

    // Index
    // BaseModel does some of the initial indexing of it's own
    static unordered_map<QString, Type*> _db; // Indexed for lookup
};

Q_DECLARE_METATYPE(Type)

#endif // TYPE_H
