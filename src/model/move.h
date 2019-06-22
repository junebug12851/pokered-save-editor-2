#ifndef MOVES_H
#define MOVES_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

#include "basemodel.h"

class Move : public BaseModel
{
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER name CONSTANT FINAL)
    Q_PROPERTY(quint8 index MEMBER index CONSTANT FINAL)
    Q_PROPERTY(quint8 power MEMBER power CONSTANT FINAL)
    Q_PROPERTY(QString type MEMBER type CONSTANT FINAL)
    Q_PROPERTY(quint8 accuracy MEMBER accuracy CONSTANT FINAL)
    Q_PROPERTY(quint8 pp MEMBER pp CONSTANT FINAL)
    Q_PROPERTY(quint8 tm MEMBER tm CONSTANT FINAL)
    Q_PROPERTY(quint8 hm MEMBER hm CONSTANT FINAL)
    Q_PROPERTY(bool glitch MEMBER glitch CONSTANT FINAL)

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
};

#endif // MOVES_H
