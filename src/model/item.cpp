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

const unordered_map<QString, Item*>& Item::db()
{
    return Item::_db;
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
        // If this is a TM
        if(el->_tm)
        {
            // Grab our TM
            auto ourVal = *el->_tm;

            // Grab their TM
            Move* theirVal = nullptr;

            for(auto targEl : Move::store())
                if(targEl->tm() && *targEl->tm() == ourVal) {
                    theirVal = targEl;
                    break;
                }

            // Deep link if found
            if(theirVal != nullptr)
                el->_toTmMove = theirVal;
        }

        // If this is a HM
        if(el->_hm)
        {
            // Grab our HM
            auto ourVal = *el->_hm;

            // Grab their HM
            Move* theirVal = nullptr;

            for(auto targEl : Move::store())
                if(targEl->hm() && *targEl->hm() == ourVal) {
                    theirVal = targEl;
                    break;
                }

            // Deep link if found
            if(theirVal != nullptr)
                el->_toHmMove = theirVal;
        }
    }
}

vector<Item*> Item::_store = vector<Item*>();
unordered_map<QString, Item*> Item::_db = unordered_map<QString, Item*>();
