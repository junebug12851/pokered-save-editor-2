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
    const Item* lookupItem(QString key);
    const Move* lookupMove(QString key);
    const Pokemon* lookupPokemon(QString key);
    const Type* lookupType(QString key);

    static void initStores();
};

#endif // POKEMONDATABASE_H
