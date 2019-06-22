#include "move.h"

Move::Move(const QString name,
           const quint8 index,
           const quint8 power,
           const QString type,
           const quint8 accuracy,
           const quint8 pp,
           const quint8 tm,
           const quint8 hm,
           const bool glitch,
           QObject* parent)
    : BaseModel(name, index, parent),
      power(power),
      type(type),
      accuracy(accuracy),
      pp(pp),
      tm(tm),
      hm(hm),
      glitch(glitch)
{}
