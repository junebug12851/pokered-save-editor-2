/*
  * Copyright 2020 Twilight
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *   http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/

#include <QQmlEngine>
#include <pse-common/utility.h>

#include "fontsearch.h"
#include "../fontsdb.h"
#include "../entries/fontdbentry.h"
#include "../fontsdb.h"

FontSearch::FontSearch()
{
  qmlRegister();
  startOver();
}

FontSearch* FontSearch::startOver()
{
  results.clear();

  // Copy elements over to begin search
  for(auto entry : FontsDB::inst()->getStore())
  {
    results.append(entry);
  }

  fontCountChanged();

  return this;
}

FontSearch* FontSearch::clear()
{
  results.clear();
  fontCountChanged();

  return this;
}

FontSearch* FontSearch::keepAnyOf(bool normal, bool control, bool picture,
                                  bool singleChar, bool multiChar, bool variable)
{
  // Rebuild from the full store, keeping any entry that matches at least one
  // enabled trait. This gives the keyboard's filters OR/union semantics ("show
  // Normal AND Control" = show both), and naturally yields an empty list when
  // nothing is enabled.
  results.clear();

  for(auto entry : FontsDB::inst()->getStore())
  {
    if((normal     && entry->normal)     ||
       (control    && entry->control)    ||
       (picture    && entry->picture)    ||
       (singleChar && entry->singleChar) ||
       (multiChar  && entry->multiChar)  ||
       (variable   && entry->variable))
      results.append(entry);
  }

  fontCountChanged();

  return this;
}

FontSearch* FontSearch::andShorthand()
{
  for(auto entry : QVector<FontDBEntry*>(results))
    if(!entry->shorthand)
      results.removeOne(entry);

  fontCountChanged();

  return this;
}

FontSearch* FontSearch::notShorthand()
{
  for(auto entry : QVector<FontDBEntry*>(results))
    if(entry->shorthand)
      results.removeOne(entry);

  fontCountChanged();

  return this;
}

FontSearch* FontSearch::andNormal()
{
  for(auto entry : QVector<FontDBEntry*>(results))
    if(!entry->normal)
      results.removeOne(entry);

  fontCountChanged();

  return this;
}

FontSearch* FontSearch::notNormal()
{
  for(auto entry : QVector<FontDBEntry*>(results))
    if(entry->normal)
      results.removeOne(entry);

  fontCountChanged();

  return this;
}

FontSearch* FontSearch::andControl()
{
  for(auto entry : QVector<FontDBEntry*>(results))
    if(!entry->control)
      results.removeOne(entry);

  fontCountChanged();

  return this;
}

FontSearch* FontSearch::notControl()
{
  for(auto entry : QVector<FontDBEntry*>(results))
    if(entry->control)
      results.removeOne(entry);

  fontCountChanged();

  return this;
}

FontSearch* FontSearch::andPicture()
{
  for(auto entry : QVector<FontDBEntry*>(results))
    if(!entry->picture)
      results.removeOne(entry);

  fontCountChanged();

  return this;
}

FontSearch* FontSearch::notPicture()
{
  for(auto entry : QVector<FontDBEntry*>(results))
    if(entry->picture)
      results.removeOne(entry);

  fontCountChanged();

  return this;
}

FontSearch* FontSearch::andSingleChar()
{
  for(auto entry : QVector<FontDBEntry*>(results))
    if(!entry->singleChar)
      results.removeOne(entry);

  fontCountChanged();

  return this;
}

FontSearch* FontSearch::notSingleChar()
{
  for(auto entry : QVector<FontDBEntry*>(results))
    if(entry->singleChar)
      results.removeOne(entry);

  fontCountChanged();

  return this;
}

FontSearch* FontSearch::andMultiChar()
{
  for(auto entry : QVector<FontDBEntry*>(results))
    if(!entry->multiChar)
      results.removeOne(entry);

  fontCountChanged();

  return this;
}

FontSearch* FontSearch::notMultiChar()
{
  for(auto entry : QVector<FontDBEntry*>(results))
    if(entry->multiChar)
      results.removeOne(entry);

  fontCountChanged();

  return this;
}

FontSearch* FontSearch::andVariable()
{
  for(auto entry : QVector<FontDBEntry*>(results))
    if(!entry->variable)
      results.removeOne(entry);

  fontCountChanged();

  return this;
}

FontSearch* FontSearch::notVariable()
{
  for(auto entry : QVector<FontDBEntry*>(results))
    if(entry->variable)
      results.removeOne(entry);

  fontCountChanged();

  return this;
}

const QVector<FontDBEntry*> FontSearch::getFonts() const
{
  return results;
}

int FontSearch::getFontCount() const
{
  return results.size();
}

const FontDBEntry* FontSearch::fontAt(const int ind) const
{
  return results.at(ind);
}

void FontSearch::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void FontSearch::qmlRegister() const
{
  static bool registered = false;
  if(registered)
    return;

  qmlRegisterUncreatableType<FontSearch>("PSE.DB.FontSearch", 1, 0, "FontSearch", "Can't instantiate in QML");
  registered = true;
}

