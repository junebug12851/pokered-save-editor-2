#ifndef MOVES_H
#define MOVES_H

#include "basemodel.h"

struct Move : public BaseModel
{
    // Move Power
    var power;

    // Move Type spelled out
    QString type;

    // Move Accuracy as an interger percent (ex: 100 == 100%)
    var accuracy;

    // Move PP
    var pp;

    // Move internal TM index
    // Please note internally HM's are also TM's so this will be present on HM's
    // as well
    var tm;

    // Actual HM Number
    var hm;

    // Is this a glitch move?
    // Glitch moves are often highly incomplete meaning
    bool glitch;

    static Move fromJson(QJsonObject obj);
};

Q_DECLARE_METATYPE(Move)

#endif // MOVES_H
