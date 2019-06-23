#include "basemodel.h"

void BaseModel::fromJson(BaseModel& model, QJsonObject obj)
{
    model.name = obj["name"].toString();
    model.index = static_cast<std::uint_least8_t>(obj["ind"].toInt());
}
