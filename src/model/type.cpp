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

vector<Type*> Type::_store = vector<Type*>();
