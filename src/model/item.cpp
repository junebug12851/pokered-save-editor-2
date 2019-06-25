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

    if(obj.contains("normal"))
        this->_normal = obj["normal"].toBool();
    else
        this->_normal.reset();

    // In the JSON we have "typical" but "common" is a far better description
    if(obj.contains("typical"))
        this->_common = obj["typical"].toBool();
    else
        this->_common.reset();
}

const optional<bool>& Item::normal()
{
    return this->_normal;
}

const optional<bool>& Item::common()
{
    return this->_common;
}
