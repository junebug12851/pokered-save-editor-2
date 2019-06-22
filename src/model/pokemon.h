#ifndef POKEMON_H
#define POKEMON_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>
#include <QVector>

#include "basemodel.h"

// To make things simpler, lets keep this to have non-constant members
// To much fighting with Qt for too long otherwise
struct LevelName {
    quint8 level;
    QString name;
};

Q_DECLARE_METATYPE(LevelName)

class Pokemon : public BaseModel
{
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER name CONSTANT FINAL)
    Q_PROPERTY(quint8 index MEMBER index CONSTANT FINAL)
    Q_PROPERTY(quint8 pokedex MEMBER pokedex CONSTANT FINAL)
    Q_PROPERTY(quint8 growthRate MEMBER growthRate CONSTANT FINAL)
    Q_PROPERTY(quint8 baseHP MEMBER baseHP CONSTANT FINAL)
    Q_PROPERTY(quint8 baseAttack MEMBER baseAttack CONSTANT FINAL)
    Q_PROPERTY(quint8 baseDefense MEMBER baseDefense CONSTANT FINAL)
    Q_PROPERTY(quint8 baseSpeed MEMBER baseSpeed CONSTANT FINAL)
    Q_PROPERTY(quint8 baseSpecial MEMBER baseSpecial CONSTANT FINAL)
    Q_PROPERTY(quint8 baseExpYield MEMBER baseExpYield CONSTANT FINAL)
    Q_PROPERTY(LevelName evolution MEMBER evolution CONSTANT FINAL)
    Q_PROPERTY(QVector<LevelName> learnedMoves MEMBER learnedMoves CONSTANT FINAL)
    Q_PROPERTY(QVector<QString> initialMoves MEMBER initialMoves CONSTANT FINAL)
    Q_PROPERTY(QVector<int> tmHm MEMBER tmHm CONSTANT FINAL)
    Q_PROPERTY(QString type1 MEMBER type1 CONSTANT FINAL)
    Q_PROPERTY(QString type2 MEMBER type2 CONSTANT FINAL)
    Q_PROPERTY(QString catchRate MEMBER catchRate CONSTANT FINAL)
    Q_PROPERTY(bool glitch MEMBER glitch CONSTANT FINAL)

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
            const QVector<LevelName> learnedMoves,
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
    const LevelName evolution;

    // Learnset
    const QVector<LevelName> learnedMoves;
    const QVector<QString> initialMoves;
    const QVector<int> tmHm;

    const QString type1;
    const QString type2;
    const QString catchRate;

    // Glitch Pokemon?
    const bool glitch;
};

#endif // POKEMON_H
