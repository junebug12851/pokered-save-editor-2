#ifndef RAWSAVEDATA_H
#define RAWSAVEDATA_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

#include "../includes/vars.h"

constexpr var16f SAV_DATA_SIZE{0x8000};

class RawSaveData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(vare* data READ data NOTIFY wholeDataChanged)

public:
    explicit RawSaveData(QObject *parent = nullptr);
    explicit RawSaveData(vare data[SAV_DATA_SIZE], QObject *parent = nullptr);

    void resetData(bool silent = false);
    void setData(vare* data, bool silent = false);
    vare* data(bool syncFirst = false);

signals:
    void wholeDataChanged(vare* data);
    void silentWholeDataChanged(vare* data);

private:
    // We want this on the stack for speed, it takes up
    // only 32KB of data or ~0.03MB. A typical stack is 1MB
    // Therefore this takes up only ~3% of the stack space
    vare _data[SAV_DATA_SIZE];
};

#endif // RAWSAVEDATA_H
