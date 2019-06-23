#ifndef MOVES_H
#define MOVES_H

#include "basemodel.h"

struct Move : public BaseModel
{
    // Move Power
    std::uint_least8_t power;

    // Move Type spelled out
    QString type;

    // Move Accuracy as an interger percent (ex: 100 == 100%)
    std::uint_least8_t accuracy;

    // Move PP
    std::uint_least8_t pp;

    // Move internal TM index
    // Please note internally HM's are also TM's so this will be present on HM's
    // as well
    std::uint_least8_t tm;

    // Actual HM Number
    std::uint_least8_t hm;

    // Is this a glitch move?
    // Glitch moves are often highly incomplete meaning
    bool glitch;

    static Move fromJson(QJsonObject obj);
};

Q_DECLARE_METATYPE(Move)

#endif // MOVES_H
