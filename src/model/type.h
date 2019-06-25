#ifndef TYPE_H
#define TYPE_H

#include "basemodel.h"

struct Type : public BaseModel
{
    Type();
    Type(const QJsonObject& obj);

    // Type only has name and index, BaseModel suffices
};

Q_DECLARE_METATYPE(Type)

#endif // TYPE_H
