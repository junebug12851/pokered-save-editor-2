#include "move.h"


Move Move::fromJson(QJsonObject obj)
{
    Move ret;
    BaseModel::fromJson(ret, obj);

    ret.power = static_cast<quint8>(obj["power"].toInt());
    ret.type = obj["type"].toString();
    ret.accuracy = static_cast<quint8>(obj["accuracy"].toInt());
    ret.pp = static_cast<quint8>(obj["pp"].toInt());
    ret.tm = static_cast<quint8>(obj["tm"].toInt());
    ret.hm = static_cast<quint8>(obj["hm"].toInt());
    ret.glitch = obj["glitch"].toBool();

    return ret;
}
