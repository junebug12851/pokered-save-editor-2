#include "type.h"

Type::Type()
{}

Type::Type(const QJsonObject &obj) :
    BaseModel (obj)
{}

TypeArr Type::store()
{
    return &Type::_store;
}

void Type::initStore(const QString& filename)
{
    BaseModel::initStore<Type>(filename, Type::_store);
}

TypeDb Type::db()
{
    return &Type::_db;
}

void Type::initDb()
{
    BaseModel::initIndex<Type>(Type::_db, Type::_store);

    // All done by BaseModel, nothing to add here
}

_TypeArr Type::_store = _TypeArr();
_TypeDb Type::_db = _TypeDb();
