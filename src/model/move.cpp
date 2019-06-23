#include "move.h"


Move Move::fromJson(QJsonObject obj)
{
    Move ret;
    BaseModel::fromJson(ret, obj);

    ret.power = static_cast<var8>(obj["power"].toInt());
    ret.type = obj["type"].toString();
    ret.accuracy = static_cast<var8>(obj["accuracy"].toInt());
    ret.pp = static_cast<var8>(obj["pp"].toInt());
    ret.tm = static_cast<var8>(obj["tm"].toInt());
    ret.hm = static_cast<var8>(obj["hm"].toInt());
    ret.glitch = obj["glitch"].toBool();

    return ret;
}
