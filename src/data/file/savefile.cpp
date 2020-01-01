#include "savefile.h"
#include "./expanded/savefileexpanded.h"

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
  memset(data, 0, SAV_DATA_SIZE);

  if(!silent) {
    wholeDataChanged(data);
    expandData();
  }
  else
    silentWholeDataChanged(data);
}

void SaveFile::setData(var8* data, bool silent)
{
  memcpy(this->data, data, SAV_DATA_SIZE);

  if(!silent) {
    expandData();
    wholeDataChanged(data);
  }
  else
    silentWholeDataChanged(data);
}

void SaveFile::flattenData()
{
  if(dataExpanded != nullptr)
    dataExpanded->save(this);
}

void SaveFile::expandData()
{
  if(dataExpanded == nullptr)
    dataExpanded = new SaveFileExpanded();

  dataExpanded->load(this);
}
