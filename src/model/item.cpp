#include "item.h"


Item Item::fromJson(QJsonObject obj)
{
    Item ret;
    BaseModel::fromJson(ret, obj);

    ret.normal = obj["normal"].toBool(false);
    ret.typical = obj["typical"].toBool(false);

    return ret;
}
