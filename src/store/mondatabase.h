#ifndef MONDATABASE_H
#define MONDATABASE_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

class MonDatabase : public QObject
{
    Q_OBJECT
public:
    explicit MonDatabase(QObject *parent = nullptr);

private:

};

#endif // MONDATABASE_H
