#ifndef POKEMONDATABASE_H
#define POKEMONDATABASE_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>
#include <QHash>
#include <QVariant>
#include <QVariantList>

#include "../model/item.h"
#include "../model/move.h"
#include "../model/pokemon.h"
#include "../model/type.h"

class PokemonDatabase : public QObject
{
    Q_OBJECT
public:
    explicit PokemonDatabase(QObject *parent = nullptr);

//    const QHash<QVariant, Item> items;
//    const QHash<QVariant, Move> move;
//    const QHash<QVariant, Pokemon> pokemon;
//    const QHash<QVariant, Type> type;

private:
    //QVariantList jsonToList(QString filename);
};

#endif // POKEMONDATABASE_H
