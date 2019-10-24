#include "item.h"
#include "move.h"

#include <QString>

Item::Item()
{}

Item::Item(QJsonObject &obj) :
    BaseModel<Item> (obj)
{
    this->init(obj);
}

optional<bool*> Item::glitch()
{
    return this->val<bool>(key_glitch);
}

optional<bool*> Item::common()
{
    return this->val<bool>(key_common);
}

optional<var8*> Item::tm()
{
    return this->val<var8>(key_tm);
}

optional<var8*> Item::hm()
{
    return this->val<var8>(key_hm);
}

optional<Move*> Item::toTmMove()
{
    return this->val<Move>(key_to_tm_move);
}

optional<Move*> Item::toHmMove()
{
    return this->val<Move>(key_to_hm_move);
}

void Item::init(QJsonObject &obj)
{
    BaseModel<Item>::init(obj);

    // This is a work around to an age-old problem
    // The items.json file was formed first and before before "glitch" was thought of
    // Instead the opposite was marked, if the item wasn't a glitch item
    // "glitch" is far easier to understand and work with so we have to essentially
    // invert the json data
    this->setVal(key_glitch, new bool(!obj["notGlitch"].toBool(false)));
    if(this->glitch() && **this->glitch() == false)
        this->_modelData.remove(key_glitch);

    if(obj.contains("common"))
        this->setVal(key_common, new bool(obj["common"].toBool()));
    if(obj.contains("tm"))
        this->setVal(key_tm, new var8(static_cast<var8>(obj["tm"].toInt())));
    if(obj.contains("hm"))
        this->setVal(key_hm, new var8(static_cast<var8>(obj["hm"].toInt())));
}

void Item::initDb()
{
    BaseModel<Item>::initDb();

    QString tmp;
    for(auto el : BaseModel<Item>::_store)
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

void Item::initDeepLink()
{
    QString tmp;
    for(auto el : _store)
    {
        if(el->tm())
        {
            tmp = "tm" + QString::number(**el->tm());
            el->setVal(key_to_tm_move, Move::db()->value(tmp));
        }

        if(el->hm())
        {
            tmp = "hm" + QString::number(**el->hm());
            el->setVal(key_to_hm_move, Move::db()->value(tmp));
        }
    }
}
