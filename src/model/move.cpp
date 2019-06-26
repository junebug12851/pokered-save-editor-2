#include "move.h"

Move::Move()
{}

Move::Move(const QJsonObject& obj) :
    BaseModel (obj)
{
    this->init(obj);
}

void Move::init(const QJsonObject &obj)
{
    BaseModel::init(obj);

    this->_toTmItem.reset();
    this->_toHmItem.reset();

    if(obj.contains("power"))
        this->_power = static_cast<vars>(obj["power"].toInt());
    else
        this->_power.reset();

    if(obj.contains("type"))
        this->_type = obj["type"].toString();
    else
        this->_type.reset();

    if(obj.contains("accuracy"))
        this->_accuracy = static_cast<vars>(obj["accuracy"].toInt());
    else
        this->_accuracy.reset();

    if(obj.contains("pp"))
        this->_pp = static_cast<vars>(obj["pp"].toInt());
    else
        this->_pp.reset();

    if(obj.contains("tm"))
        this->_tm = static_cast<vars>(obj["tm"].toInt());
    else
        this->_tm.reset();

    if(obj.contains("hm"))
        this->_hm = static_cast<vars>(obj["hm"].toInt());
    else
        this->_hm.reset();

    if(obj.contains("glitch"))
        this->_glitch = obj["glitch"].toBool();
    else
        this->_glitch.reset();
}

const optional<vars>& Move::power()
{
    return this->_power;
}

const optional<QString>& Move::type()
{
    return this->_type;
}

const optional<vars>& Move::accuracy()
{
    return this->_accuracy;
}

const optional<vars>& Move::pp()
{
    return this->_pp;
}

const optional<vars>& Move::tm()
{
    return this->_tm;
}

const optional<vars>& Move::hm()
{
    return this->_hm;
}

const optional<bool>& Move::glitch()
{
    return this->_glitch;
}

const optional<Item*>& Move::toTmItem()
{
    return this->_toTmItem;
}

const optional<Item*>& Move::toHmItem()
{
    return this->_toHmItem;
}

const vector<Move*>& Move::store()
{
    return Move::_store;
}

const unordered_map<QString, Move*>& Move::db()
{
    return Move::_db;
}

void Move::initStore(const QString& filename)
{
    BaseModel::initStore<Move>(filename, Move::_store);
}

void Move::initDb()
{
    BaseModel::initIndex<Move>(Move::_db, Move::_store);

    QString tmp;
    for(auto el : Move::_store)
    {
        if(el->_tm)
        {
            vars val = *el->_tm;

            tmp = "tm";
            tmp = tmp.append(QString::number(val));
            Move::_db[tmp] = el;

            if(val < 10)
            {
                tmp = "tm0";
                tmp = tmp.append(QString::number(val));
                Move::_db[tmp] = el;
            }
        }

        if(el->_hm)
        {
            vars val = *el->_hm;
            tmp = "hm";
            tmp = tmp.append(QString::number(val));
            Move::_db[tmp] = el;

            if(val < 10)
            {
                tmp = "hm0";
                tmp = tmp.append(QString::number(val));
                Move::_db[tmp] = el;
            }
        }
    }
}

vector<Move*> Move::_store = vector<Move*>();
unordered_map<QString, Move*> Move::_db = unordered_map<QString, Move*>();
