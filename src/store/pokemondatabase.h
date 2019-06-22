#ifndef POKEMONDATABASE_H
#define POKEMONDATABASE_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

class PokemonDatabase : public QObject
{
    Q_OBJECT
public:
    explicit PokemonDatabase(QObject *parent = nullptr);
};

#endif // POKEMONDATABASE_H
