#include "item.h"

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
        this->_glitch.reset();

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

const vector<Item*>& Item::store()
{
    return Item::_store;
}

void Item::initStore(const QString& filename)
{
    BaseModel::initStore<Item>(filename, Item::_store);
}

vector<Item*> Item::_store = vector<Item*>();
