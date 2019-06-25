#include "item.h"

Item::Item()
{}

Item::Item(const QJsonObject& obj) :
    BaseModel (obj)
{
    if(obj.contains("normal"))
        this->normal = obj["normal"].toBool();

    // In the JSON we have "typical" but "common" is a far better description
    if(obj.contains("typical"))
        this->common = obj["typical"].toBool();
}
