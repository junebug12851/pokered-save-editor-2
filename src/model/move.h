#ifndef MOVES_H
#define MOVES_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

#include <QVector>

#include "basemodel.h"

class Move : public BaseModel
{
    Q_OBJECT
public:
    explicit Move(
            const QString name,
            const quint8 index,
            const quint8 power,
            const QString type,
            const quint8 accuracy,
            const quint8 pp,
            const quint8 tm,
            const quint8 hm,
            const bool glitch,
            QObject *parent = nullptr);

    // Move Power
    const quint8 power;

    // Move Type spelled out
    const QString type;

    // Move Accuracy as an interger percent (ex: 100 == 100%)
    const quint8 accuracy;

    // Move PP
    const quint8 pp;

    // Move internal TM index
    // Please note internally HM's are also TM's so this will be present on HM's
    // as well
    const quint8 tm;

    // Actual HM Number
    const quint8 hm;

    // Is this a glitch move?
    // Glitch moves are often highly incomplete meaning
    const bool glitch;

    static const QVector<Move> store;
};

#endif // MOVES_H
