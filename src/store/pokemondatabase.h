#ifndef POKEMONDATABASE_H
#define POKEMONDATABASE_H

#include "../model/item.h"
#include "../model/move.h"
#include "../model/pokemon.h"
#include "../model/type.h"

#include <QQmlContext>

// This is for Qt Quick to interface with the databases
class PokemonDatabase
{
public:
    Item* lookupItem(QString key);
    Move* lookupMove(QString key);
    Pokemon* lookupPokemon(QString key);
    Type* lookupType(QString key);

    static void initStores();
};

#endif // POKEMONDATABASE_H
