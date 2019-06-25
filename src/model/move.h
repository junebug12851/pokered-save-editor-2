#ifndef MOVES_H
#define MOVES_H

#include "basemodel.h"

struct Move : public BaseModel
{
    Move();
    Move(const QJsonObject& obj);

    // Move Power
    optional<vars> power;

    // Move Type spelled out
    optional<QString> type;

    // Move Accuracy as an interger percent (ex: 100 == 100%)
    optional<vars> accuracy;

    // Move PP
    optional<vars> pp;

    // Move internal TM index
    // Please note internally HM's are also TM's so this will be present on HM's
    // as well
    optional<vars> tm;

    // Actual HM Number
    optional<vars> hm;

    // Is this a glitch move?
    // Glitch moves are often highly incomplete meaning
    optional<bool> glitch;
};

Q_DECLARE_METATYPE(Move)

#endif // MOVES_H
