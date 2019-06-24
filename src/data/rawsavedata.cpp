#include "rawsavedata.h"

RawSaveData::RawSaveData(QObject *parent)
    : QObject(parent)
{
    // Zero out data and notify
    this->resetData();
}

RawSaveData::RawSaveData(vare data[SAV_DATA_SIZE], QObject *parent)
    : QObject(parent)
{
    // Init data and notify
    this->setData(data);
}

void RawSaveData::resetData(bool silent)
{
    memset(this->_data, 0, SAV_DATA_SIZE);

    if(!silent)
        this->wholeDataChanged(this->_data);
    else
        this->silentWholeDataChanged(this->_data);
}

void RawSaveData::setData(vare *data, bool silent)
{
    memcpy(this->_data, data, SAV_DATA_SIZE);

    if(!silent)
        this->wholeDataChanged(this->_data);
    else
        this->silentWholeDataChanged(this->_data);
}

vare *RawSaveData::data(bool syncFirst)
{
    //@TODO implement syncFirst
    return this->_data;
}
