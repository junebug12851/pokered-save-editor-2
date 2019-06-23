#ifndef RAWSAVEDATA_H
#define RAWSAVEDATA_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

#include "../includes/vars.h"

#define SAV_DATA_SIZE 0x8000

class RawSaveData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(var8e* data READ data NOTIFY wholeDataChanged)

public:
    explicit RawSaveData(QObject *parent = nullptr);
    explicit RawSaveData(var8e data[SAV_DATA_SIZE], QObject *parent = nullptr);

    void resetData(bool silent = false);
    void setData(var8e* data, bool silent = false);
    var8e* data(bool syncFirst = false);

signals:
    void wholeDataChanged(var8e* data);
    void silentWholeDataChanged(var8e* data);

private:
    var8e _data[SAV_DATA_SIZE];
};

#endif // RAWSAVEDATA_H
