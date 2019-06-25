#include "move.h"

Move::Move()
{}

Move::Move(const QJsonObject& obj) :
    BaseModel (obj)
{
    if(obj.contains("power"))
        this->power = static_cast<vars>(obj["power"].toInt());

    if(obj.contains("type"))
        this->type = obj["type"].toString();

    if(obj.contains("accuracy"))
        this->accuracy = static_cast<vars>(obj["accuracy"].toInt());

    if(obj.contains("pp"))
        this->pp = static_cast<vars>(obj["pp"].toInt());

    if(obj.contains("tm"))
        this->tm = static_cast<vars>(obj["tm"].toInt());

    if(obj.contains("hm"))
        this->hm = static_cast<vars>(obj["hm"].toInt());

    if(obj.contains("glitch"))
        this->glitch = obj["glitch"].toBool();
}
