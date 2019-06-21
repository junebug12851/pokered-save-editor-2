#ifndef POKEMON_H
#define POKEMON_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>
#include <QVector>

#include "basemodel.h"

class MoveLearnset : public QObject {
    Q_OBJECT

public:
    explicit MoveLearnset(
            const quint8 level,
            const QString name,
            QObject *parent = nullptr
        );

    explicit MoveLearnset(
            const MoveLearnset& obj,
            QObject *parent = nullptr
        );

    const quint8 level;
    const QString name;
};

class Evolution  : public QObject {
    Q_OBJECT

public:
    explicit Evolution(
            const quint8 level,
            const QString name,
            QObject *parent = nullptr
        );
    explicit Evolution(
            const Evolution& obj,
            QObject *parent = nullptr
        );

    const quint8 level;
    const QString name;
};

class Pokemon : public BaseModel
{
    Q_OBJECT
public:
    explicit Pokemon(
            const QString name,
            const quint8 index,
            const quint8 pokedex,
            const quint8 growthRate,
            const quint8 baseHP,
            const quint8 baseAttack,
            const quint8 baseDefense,
            const quint8 baseSpeed,
            const quint8 baseSpecial,
            const quint8 baseExpYield,
            const Evolution evolution,
            const QVector<MoveLearnset> learnedMoves,
            const QVector<QString> initialMoves,
            const QVector<int> tmHm,
            const QString type1,
            const QString type2,
            const QString catchRate,
            const bool glitch,
            QObject *parent = nullptr
        );

    // Pokemon Pokedex Index
    const quint8 pokedex;

    // Pokemon Growth Rate
    // How big or small is the exp range to reach max level
    // The number here is the internal growth rate index, in other words the
    // amount of the number (bigger or smaller) has no correlationto the growth
    // rate
    const quint8 growthRate;

    // Base Stats
    const quint8 baseHP;
    const quint8 baseAttack;
    const quint8 baseDefense;
    const quint8 baseSpeed;
    const quint8 baseSpecial;
    const quint8 baseExpYield;

    // Evolution
    const Evolution evolution;

    // Learnset
    const QVector<MoveLearnset> learnedMoves;
    const QVector<QString> initialMoves;
    const QVector<int> tmHm;

    const QString type1;
    const QString type2;
    const QString catchRate;

    // Glitch Pokemon?
    const bool glitch;

    static const QVector<Pokemon> store;
};

#endif // POKEMON_H
