#include "basemodel.h"

BaseModel::BaseModel()
{}

BaseModel::BaseModel(const QJsonObject& obj)
{
    if(obj.contains("name"))
        this->name = obj["name"].toString();

    if(obj.contains("ind"))
        this->index = static_cast<vars>(obj["ind"].toInt());
}
