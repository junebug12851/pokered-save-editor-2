#ifndef POKEMONEVOLUTION_H
#define POKEMONEVOLUTION_H

#include "basemodel.h"

class Item;
class Pokemon;

class PokemonEvolution : public BaseModel<PokemonEvolution>
{
public:
    // Allow Pokemon to access this class's private variables
    friend class Pokemon;

    enum keys: var8 {
        // Continue where the parent left off
        key_level = BaseModel<PokemonEvolution>::keystore_size,
        key_item,
        key_to_item,
        key_to_pokemon,
        key_parent,
        keystore_size
    };

    PokemonEvolution();
    PokemonEvolution(QJsonObject& obj, Pokemon* parent);

    optional<var8*> level();
    optional<QString*> item();

    optional<Item*> toItem();
    Pokemon* toPokemon();
    Pokemon* parent();

    // Deep link items from this store with items in other stores
    void initDeepLink();
private:
    // Init Model
    void init(QJsonObject& obj, Pokemon* parent);
};

#endif // POKEMONEVOLUTION_H
