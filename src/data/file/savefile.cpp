#include "savefile.h"

SaveFile::SaveFile(QObject* parent)
    : QObject(parent)
{
    // Zero out data and notify
    resetData();
}

SaveFile::SaveFile(var8* data, QObject* parent)
    : QObject(parent)
{
    // Init data and notify
    setData(data);
}

void SaveFile::resetData(bool silent)
{
    memset(_data, 0, SAV_DATA_SIZE);

    if(!silent)
        wholeDataChanged(_data);
    else
        silentWholeDataChanged(_data);
}

void SaveFile::setData(var8* data, bool silent)
{
    memcpy(_data, data, SAV_DATA_SIZE);

    if(!silent)
        wholeDataChanged(_data);
    else
        silentWholeDataChanged(_data);
}

var8* SaveFile::data(bool syncFirst)
{
    //@TODO implement syncFirst
    return _data;
}
