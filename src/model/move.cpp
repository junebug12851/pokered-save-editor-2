#include "move.h"
#include "item.h"
#include "type.h"

Move::Move()
{}

Move::Move(QJsonObject &obj) :
    BaseModel<Move> (obj)
{
    this->init(obj);
}

const optional<vars> Move::power()
{
    return this->val<vars>(key_power);
}

const optional<QString> Move::type()
{
    return this->val<QString>(key_type);
}

const optional<vars> Move::accuracy()
{
    return this->val<vars>(key_accuracy);
}

const optional<vars> Move::pp()
{
    return this->val<vars>(key_pp);
}

const optional<vars> Move::tm()
{
    return this->val<vars>(key_tm);
}

const optional<vars> Move::hm()
{
    return this->val<vars>(key_hm);
}

const optional<bool> Move::glitch()
{
    return this->val<bool>(key_glitch);
}

const optional<Item *> Move::toTmItem()
{
    return this->_toTmItem;
}

const optional<Item *> Move::toHmItem()
{
    return this->_toHmItem;
}

const optional<Type *> Move::toType()
{
    return this->_toType;
}

void Move::init(QJsonObject &obj)
{
    BaseModel<Move>::init(obj);

    if(obj.contains("power"))
        this->setVal(key_power, obj["power"].toInt());
    if(obj.contains("type"))
        this->setVal(key_power, obj["type"].toString());
    if(obj.contains("accuracy"))
        this->setVal(key_power, obj["accuracy"].toInt());
    if(obj.contains("pp"))
        this->setVal(key_power, obj["pp"].toInt());
    if(obj.contains("tm"))
        this->setVal(key_power, obj["tm"].toInt());
    if(obj.contains("hm"))
        this->setVal(key_power, obj["hm"].toInt());
    if(obj.contains("glitch"))
        this->setVal(key_power, obj["glitch"].toBool());
}

void Move::initDb()
{
    BaseModel<Move>::initDb();

    QString tmp;
    for(auto& el : _store)
    {
        if(el.tm())
        {
            tmp = "tm" + QString::number(*el.tm());
            _db.insert(tmp, &el);
        }

        if(el.hm())
        {
            tmp = "hm" + QString::number(*el.hm());
            _db.insert(tmp, &el);
        }
    }
}

void Move::initDeepLink()
{
    QString tmp;
    for(auto& el : _store)
    {
        if(el.tm())
        {
            tmp = "tm" + QString::number(*el.tm());
            el._toTmItem = Item::db()->value(tmp);
        }

        if(el.hm())
        {
            tmp = "hm" + QString::number(*el.hm());
            el._toHmItem = Item::db()->value(tmp);
        }

        if(el.type())
            el._toType = Type::db()->value(tmp);
    }
}
