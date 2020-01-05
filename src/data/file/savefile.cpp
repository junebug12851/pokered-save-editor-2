#include "savefile.h"
#include "./expanded/savefileexpanded.h"
#include "./savefileiterator.h"
#include "./savefiletoolset.h"

SaveFile::SaveFile(QObject* parent)
  : QObject(parent)
{
  // One-Time Init Data in order of dependencies
  data = new var8[SAV_DATA_SIZE];
  toolset = new SaveFileToolset(this);
  dataExpanded = new SaveFileExpanded(this);

  // Perform post-init stuff
  resetData();
}

SaveFile::~SaveFile()
{
  delete dataExpanded;
  delete toolset;
  delete data;
}

SaveFileIterator* SaveFile::iterator()
{
  return new SaveFileIterator(this);
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
  dataExpanded->save(this);
}

void SaveFile::expandData()
{
  dataExpanded->load(this);
}

void SaveFile::eraseExpansion()
{
  dataExpanded->reset();
}

void SaveFile::randomizeExpansion()
{
  dataExpanded->randomize();
}
