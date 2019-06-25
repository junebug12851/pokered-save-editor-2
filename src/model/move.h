#ifndef MOVES_H
#define MOVES_H

#include "basemodel.h"

class Move : public BaseModel
{
public:
    Move();
    Move(const QJsonObject& obj);

    void init(const QJsonObject& obj);

    const optional<vars>& power();
    const optional<QString>& type();
    const optional<vars>& accuracy();
    const optional<vars>& pp();
    const optional<vars>& tm();
    const optional<vars>& hm();
    const optional<bool>& glitch();

private:
    // Move Power
    optional<vars> _power;

    // Move Type spelled out
    optional<QString> _type;

    // Move Accuracy as an interger percent (ex: 100 == 100%)
    optional<vars> _accuracy;

    // Move PP
    optional<vars> _pp;

    // Move internal TM index
    // Please note internally HM's are also TM's so this will be present on HM's
    // as well
    optional<vars> _tm;

    // Actual HM Number
    optional<vars> _hm;

    // Is this a glitch move?
    // Glitch moves are often highly incomplete meaning
    optional<bool> _glitch;
};

Q_DECLARE_METATYPE(Move)

#endif // MOVES_H
