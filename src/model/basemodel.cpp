#include "basemodel.h"

BaseModel::BaseModel()
{}

BaseModel::BaseModel(const QJsonObject& obj)
{
    this->init(obj);
}

void BaseModel::init(const QJsonObject &obj)
{
    if(obj.contains("name"))
        this->_name = obj["name"].toString();
    else
        this->_name.reset();

    if(obj.contains("ind"))
        this->_index = static_cast<vars>(obj["ind"].toInt());
    else
        this->_index.reset();
}

const optional<QString>& BaseModel::name()
{
    return this->_name;
}

const optional<vars>& BaseModel::index()
{
    return this->_index;
}
