#ifndef RAWSAVEDATA_H
#define RAWSAVEDATA_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

#define SAV_DATA_SIZE 0x8000

class RawSaveData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint8* data READ data NOTIFY wholeDataChanged)

public:
    explicit RawSaveData(QObject *parent = nullptr);
    explicit RawSaveData(quint8 data[SAV_DATA_SIZE], QObject *parent = nullptr);

    void resetData(bool silent = false);
    void setData(quint8* data, bool silent = false);
    quint8* data(bool syncFirst = false);

signals:
    void wholeDataChanged(quint8* data);
    void silentWholeDataChanged(quint8* data);

private:
    quint8 _data[SAV_DATA_SIZE];
};

#endif // RAWSAVEDATA_H
