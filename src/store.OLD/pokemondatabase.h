#ifndef POKEMONDATABASE_H
#define POKEMONDATABASE_H

#include "../model/item.h"
#include "../model/move.h"
#include "../model/pokemon.h"
#include "../model/type.h"

#include <QQmlContext>

// This is for Qt Quick to interface with the databases
class PokemonDatabase : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE Item* lookupItem(QString key);
    Q_INVOKABLE Move* lookupMove(QString key);
    Q_INVOKABLE Pokemon* lookupPokemon(QString key);
    Q_INVOKABLE Type* lookupType(QString key);

    static void initStores();
    static void qmlRegisterModels();
};

#endif // POKEMONDATABASE_H
