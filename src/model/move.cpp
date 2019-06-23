#include "move.h"


Move Move::fromJson(QJsonObject obj)
{
    Move ret;
    BaseModel::fromJson(ret, obj);

    ret.power = static_cast<std::uint_least8_t>(obj["power"].toInt());
    ret.type = obj["type"].toString();
    ret.accuracy = static_cast<std::uint_least8_t>(obj["accuracy"].toInt());
    ret.pp = static_cast<std::uint_least8_t>(obj["pp"].toInt());
    ret.tm = static_cast<std::uint_least8_t>(obj["tm"].toInt());
    ret.hm = static_cast<std::uint_least8_t>(obj["hm"].toInt());
    ret.glitch = obj["glitch"].toBool();

    return ret;
}
