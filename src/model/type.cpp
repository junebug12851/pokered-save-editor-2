#include "type.h"

Type::Type()
{}

Type::Type(QJsonObject &obj) :
    BaseModel<Type> (obj)
{
    this->init(obj);
}

void Type::init(QJsonObject &obj)
{
    BaseModel<Type>::init(obj);
}

void Type::initDb()
{
    BaseModel<Type>::initDb();
}
