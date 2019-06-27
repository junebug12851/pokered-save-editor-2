#include "pokemondatabase.h"

// This will take a long time, run at program start
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
