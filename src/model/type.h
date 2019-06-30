#ifndef TYPE_H
#define TYPE_H

#include "basemodel.h"

class Type : public BaseModel<Type>
{
public:
    enum keys: var {
        // Continue where the parent left off
        keystore_size = BaseModel<Type>::keystore_size
    };
    Type();
    Type(QJsonObject& obj);

    // Indexes a store to a db for speedy lookup
    static void initDb();
private:
    // Init Model
    void init(QJsonObject& obj);
};

Q_DECLARE_METATYPE(Type)
Q_DECLARE_METATYPE(Type*)

#endif // TYPE_H
