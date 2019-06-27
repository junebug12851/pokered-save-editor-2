#include "item.h"
#include "move.h"

Item::Item()
{}

Item::Item(const QJsonObject& obj) :
    BaseModel (obj)
{
    this->init(obj);
}

void Item::init(const QJsonObject& obj)
{
    BaseModel::init(obj);

    this->_toTmMove.reset();
    this->_toHmMove.reset();

    if(obj.contains("normal"))
        this->_glitch = !obj["normal"].toBool();
    else
        this->_glitch = true;

    // In the JSON we have "typical" but "common" is a far better description
    if(obj.contains("typical"))
        this->_common = obj["typical"].toBool();
    else
        this->_common.reset();

    if(obj.contains("tm"))
        this->_tm = static_cast<vars>(obj["tm"].toInt());
    else
        this->_tm.reset();

    if(obj.contains("hm"))
        this->_hm = static_cast<vars>(obj["hm"].toInt());
    else
        this->_hm.reset();
}

const optional<bool>& Item::glitch()
{
    return this->_glitch;
}

const optional<bool>& Item::common()
{
    return this->_common;
}

const optional<vars>& Item::tm()
{
    return this->_tm;
}

const optional<vars>& Item::hm()
{
    return this->_hm;
}

const optional<Move*>& Item::toTmMove()
{
    return this->_toTmMove;
}

const optional<Move*>& Item::toHmMove()
{
    return this->_toHmMove;
}

const Item *Item::lookupEntry(QString name)
{
    return Item::_db.at(name);
}

ItemArr Item::store()
{
    return &Item::_store;
}

ItemDb Item::db()
{
    return &Item::_db;
}

void Item::initStore(const QString& filename)
{
    BaseModel::initStore<Item>(filename, Item::_store);
}

void Item::initDb()
{
    BaseModel::initIndex<Item>(Item::_db, Item::_store);

    QString tmp;
    for(auto el : Item::_store)
    {
        if(el->_tm)
        {
            vars val = *el->_tm;

            tmp = "tm";
            tmp = tmp.append(QString::number(val));
            Item::_db[tmp] = el;

            if(val < 10)
            {
                tmp = "tm0";
                tmp = tmp.append(QString::number(val));
                Item::_db[tmp] = el;
            }
        }

        if(el->_hm)
        {
            vars val = *el->_hm;
            tmp = "hm";
            tmp = tmp.append(QString::number(val));
            Item::_db[tmp] = el;

            if(val < 10)
            {
                tmp = "hm0";
                tmp = tmp.append(QString::number(val));
                Item::_db[tmp] = el;
            }
        }
    }
}

// Deep links TM & HM items to their move counterpart
void Item::initDeepLink()
{
    for(auto el : Item::_store)
    {
        // If this is a TM, assign it
        // Will crash if not found which is needed anyways
        if(el->_tm)
            el->_toTmMove = Move::db()->at("tm" + QString::number(*el->_tm));

        // If this is a TM, assign it
        // Will crash if not found which is needed anyways
        if(el->_hm)
            el->_toHmMove = Move::db()->at("hm" + QString::number(*el->_hm));
    }
}

_ItemArr Item::_store = _ItemArr();
_ItemDb Item::_db = _ItemDb();
