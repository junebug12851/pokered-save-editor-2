#include "item.h"

#include "move.h"

Item::Item()
{}

Item::Item(QJsonObject &obj) :
    BaseModel<Item> (obj)
{
    this->init(obj);
}

const optional<bool> Item::glitch()
{
    return this->val<bool>(key_glitch);
}

const optional<bool> Item::common()
{
    return this->val<bool>(key_common);
}

const optional<var> Item::tm()
{
    return this->val<var>(key_tm);
}

const optional<var> Item::hm()
{
    return this->val<var>(key_hm);
}

const optional<Move*> Item::toTmMove()
{
    return this->valVoidPtr<Move*>(key_to_tm_move);
}

const optional<Move*> Item::toHmMove()
{
    return this->valVoidPtr<Move*>(key_to_hm_move);
}

void Item::init(QJsonObject &obj)
{
    BaseModel<Item>::init(obj);

    if(obj.contains("glitch"))
        this->setVal(key_glitch, obj["glitch"].toBool());
    if(obj.contains("common"))
        this->setVal(key_common, obj["common"].toBool());
    if(obj.contains("tm"))
        this->setVal(key_tm, obj["tm"].toInt());
    if(obj.contains("hm"))
        this->setVal(key_hm, obj["hm"].toInt());
}

void Item::initDb()
{
    BaseModel<Item>::initDb();

    QString tmp;
    for(auto& el : BaseModel<Item>::_store)
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

void Item::initDeepLink()
{
    QString tmp;
    for(auto& el : _store)
    {
        if(el.tm())
        {
            tmp = "tm" + QString::number(*el.tm());
            el.setValVoidPtr<Move*>(key_to_tm_move, Move::db()->value(tmp));
        }

        if(el.hm())
        {
            tmp = "hm" + QString::number(*el.hm());
            el.setValVoidPtr<Move*>(key_to_hm_move, Move::db()->value(tmp));
        }
    }
}
