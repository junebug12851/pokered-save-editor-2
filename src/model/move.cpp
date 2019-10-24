#include "move.h"
#include "item.h"
#include "type.h"

#include <QString>

Move::Move()
{}

Move::Move(QJsonObject &obj) :
    BaseModel<Move> (obj)
{
    this->init(obj);
}

optional<var8*> Move::power()
{
    return this->val<var8>(key_power);
}

optional<QString*> Move::type()
{
    return this->val<QString>(key_type);
}

optional<var8*> Move::accuracy()
{
    return this->val<var8>(key_accuracy);
}

optional<var8*> Move::pp()
{
    return this->val<var8>(key_pp);
}

optional<var8*> Move::tm()
{
    return this->val<var8>(key_tm);
}

optional<var8*> Move::hm()
{
    return this->val<var8>(key_hm);
}

optional<bool*> Move::glitch()
{
    return this->val<bool>(key_glitch);
}

optional<Item*> Move::toTmItem()
{
    return this->val<Item>(key_to_tm_item);
}

optional<Item*> Move::toHmItem()
{
    return this->val<Item>(key_to_hm_item);
}

optional<Type*> Move::toType()
{
    return this->val<Type>(key_to_type);
}

void Move::init(QJsonObject &obj)
{
    BaseModel<Move>::init(obj);

    if(obj.contains("power"))
        this->setVal(key_power, new var8(static_cast<var8>(obj["power"].toInt())));
    if(obj.contains("type"))
        this->setVal(key_power, new QString(obj["type"].toString()));
    if(obj.contains("accuracy"))
        this->setVal(key_power, new var8(static_cast<var8>(obj["accuracy"].toInt())));
    if(obj.contains("pp"))
        this->setVal(key_power, new var8(static_cast<var8>(obj["pp"].toInt())));
    if(obj.contains("tm"))
        this->setVal(key_power, new var8(static_cast<var8>(obj["tm"].toInt())));
    if(obj.contains("hm"))
        this->setVal(key_power, new var8(static_cast<var8>(obj["hm"].toInt())));
    if(obj.contains("glitch"))
        this->setVal(key_power, new bool(obj["glitch"].toBool()));
}

void Move::initDb()
{
    BaseModel<Move>::initDb();

    QString tmp;
    for(auto el : _store)
    {
        if(el->tm())
        {
            tmp = "tm" + QString::number(**el->tm());
            _db.insert(tmp, el);
        }

        if(el->hm())
        {
            tmp = "hm" + QString::number(**el->hm());
            _db.insert(tmp, el);
        }
    }
}

void Move::initDeepLink()
{
    QString tmp;
    for(auto el : _store)
    {
        if(el->tm())
        {
            tmp = "tm" + QString::number(**el->tm());
            el->setVal(key_to_tm_item, Item::db()->value(tmp));
        }

        if(el->hm())
        {
            tmp = "hm" + QString::number(**el->hm());
            el->setVal(key_to_hm_item, Item::db()->value(tmp));
        }

        if(el->type())
            el->setVal(key_to_type, Type::db()->value(tmp));
    }
}
