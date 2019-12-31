#ifndef RAWSAVEDATA_H
#define RAWSAVEDATA_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

#include "../../common/types.h"

constexpr var16 SAV_DATA_SIZE{0x8000};

class SaveFile : public QObject
{
    Q_OBJECT
    Q_PROPERTY(var8* data READ data NOTIFY wholeDataChanged)

public:
    explicit SaveFile(QObject *parent = nullptr);
    explicit SaveFile(var8 data[SAV_DATA_SIZE], QObject *parent = nullptr);

    void resetData(bool silent = false);
    void setData(var8* data, bool silent = false);
    var8* data(bool syncFirst = false);

signals:
    void wholeDataChanged(var8* data);
    void silentWholeDataChanged(var8* data);

private:
    // Actual SAV Data to be written to file
    var8* _data = new var8[SAV_DATA_SIZE];
};

#endif // RAWSAVEDATA_H
