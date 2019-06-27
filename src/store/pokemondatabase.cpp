#include <QQmlEngine>

#include "pokemondatabase.h"

// This will take a long time, run at program start
Item *PokemonDatabase::lookupItem(QString key)
{
    return Item::db()->at(key);
}

Move *PokemonDatabase::lookupMove(QString key)
{
    return Move::db()->at(key);
}

Pokemon *PokemonDatabase::lookupPokemon(QString key)
{
    return Pokemon::db()->at(key);
}

Type *PokemonDatabase::lookupType(QString key)
{
    return Type::db()->at(key);
}

void PokemonDatabase::initStores()
{
    // Create array of models from JSON file
    // This takes a while
    Item::initStore(":/data/items.json");
    Move::initStore(":/data/moves.json");
    Pokemon::initStore(":/data/pokemon.json");
    Type::initStore(":/data/types.json");

    // Index models into a seperate db for speedy lookup
    // This takes 2-4 times longer or more
    Item::initDb();
    Move::initDb();
    Pokemon::initDb();
    Type::initDb();

    // Deep link store items to one another
    // Relatively fast but will crash if anything from any step above went wrong
    // or didn't happen
    Item::initDeepLink();
    Move::initDeepLink();
    Pokemon::initDeepLink();
}

void PokemonDatabase::qmlRegisterModels()
{
    qmlRegisterInterface<Item>("DBItem");
    qmlRegisterInterface<Move>("DBMove");
    qmlRegisterInterface<Pokemon>("DBPokemon");
    qmlRegisterInterface<Type>("DBType");

    qmlRegisterSingletonType<PokemonDatabase>("pse", 1, 0, "DB", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
          Q_UNUSED(engine)
          Q_UNUSED(scriptEngine)

          PokemonDatabase *inst = new PokemonDatabase();
          return inst;
      });

//    qmlRegisterType<PokemonDatabase>("pse", 1, 0, "DB");
//    qmlRegisterType<Pokemon>("pse", 1, 0, "Pokemon");
}
