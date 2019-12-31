#ifndef RAWSAVEDATA_H
#define RAWSAVEDATA_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

#include "../../common/types.h"
#include "./savefilecommon.h"

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
    // We want this on the stack for speed, it takes up
    // only 32KB of data or ~0.03MB. A typical stack is 1MB
    // Therefore this takes up only ~3% of the stack space

    // Was going to move it off the stack but given how often it
    // would change it's best to leave it on the stack so we're not dealing
    // with delete[] and potential memory leaks
    var8e _data[SAV_DATA_SIZE];
};

#endif // RAWSAVEDATA_H
