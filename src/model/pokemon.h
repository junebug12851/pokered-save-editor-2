#ifndef POKEMON_H
#define POKEMON_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>
#include <vector>

using namespace std;

#include "basemodel.h"

class LevelName : public QObject {
    Q_OBJECT

public:
    explicit LevelName(
            const quint8 level,
            const QString name,
            QObject *parent = nullptr
        );

    explicit LevelName(
            const LevelName& obj,
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
            const LevelName evolution,
            const vector<LevelName> learnedMoves,
            const vector<QString> initialMoves,
            const vector<int> tmHm,
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
    const LevelName evolution;

    // Learnset
    const vector<LevelName> learnedMoves;
    const vector<QString> initialMoves;
    const vector<int> tmHm;

    const QString type1;
    const QString type2;
    const QString catchRate;

    // Glitch Pokemon?
    const bool glitch;
};

#endif // POKEMON_H
