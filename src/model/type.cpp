#include "type.h"

Type::Type()
{}

Type::Type(const QJsonObject &obj) :
    BaseModel (obj)
{}

const vector<Type*>& Type::store()
{
    return Type::_store;
}

void Type::initStore(const QString& filename)
{
    BaseModel::initStore<Type>(filename, Type::_store);
}

const unordered_map<QString, Type*>& Type::db()
{
    return Type::_db;
}

void Type::initDb()
{
    BaseModel::initIndex<Type>(Type::_db, Type::_store);

    // All done by BaseModel, nothing to add here
}

vector<Type*> Type::_store = vector<Type*>();
unordered_map<QString, Type*> Type::_db = unordered_map<QString, Type*>();
