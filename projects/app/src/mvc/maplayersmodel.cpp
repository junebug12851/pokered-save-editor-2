/*
  * Copyright 2026 Twilight
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
#include "maplayersmodel.h"

#include "mapmodel.h"
#include "../engine/mapengine.h"

namespace {

/// The three groups, in the order they read: what helps you SEE (guides), what the map MEANS
/// (the semantic overlays), and what the GAME is doing right now (the player and his two boxes).
enum Group { GuidesGroup = 0, ComponentsGroup = 1, GameViewGroup = 2, GroupCount = 3 };

const char* groupKey(int g)
{
  switch (g) {
  case GuidesGroup:   return "guides";
  // ⚠️ The KEY stays "meaning" -- it is a stable id the tests and the DEBUG harness address the
  // group by. Only the NAME changed ("Meaning" -> "Components", Twilight 2026-07-13).
  case ComponentsGroup: return "meaning";
  case GameViewGroup: return "gameview";
  default:            break;
  }
  return "";
}

QString groupName(int g)
{
  switch (g) {
  case GuidesGroup:   return QObject::tr("Guides");
  case ComponentsGroup: return QObject::tr("Components");
  case GameViewGroup: return QObject::tr("Game View");
  default:            break;
  }
  return QString();
}

QString groupBlurb(int g)
{
  switch (g) {
  case GuidesGroup:
    return QObject::tr("Lines that help you see the map's shape. None of it is in the save.");
  case ComponentsGroup:
    return QObject::tr("What the map MEANS — a wall and a floor are just two pictures until "
                       "something says which is which.");
  case GameViewGroup:
    return QObject::tr("What the Game Boy is drawing right now: the player, everyone else on the "
                       "map, the screen he sees, and the patch of map it redraws around him.");
  default:
    break;
  }
  return QString();
}

} // namespace

MapLayersModel::MapLayersModel(MapModel* map, QObject* parent)
  : QAbstractListModel(parent), map(map)
{
  expanded.fill(true, GroupCount);
  buildAll();
  rebuild();

  // The map changed: which layers even APPLY changes with it (a map with no grass should say so
  // rather than switch on an empty overlay), so every row's "applies" is stale.
  if (map != nullptr) {
    connect(map, &MapModel::changed, this, &MapLayersModel::refreshAll);
    connect(map, &MapModel::overlayChanged, this, &MapLayersModel::refreshAll);
  }
}

// ── The flattened tree ────────────────────────────────────────────────────────

void MapLayersModel::buildAll()
{
  allRows.clear();

  auto group = [&](int g) {
    Row r;
    r.key = QString::fromLatin1(groupKey(g));
    r.name = groupName(g);
    r.description = groupBlurb(g);
    r.isGroup = true;
    r.group = g;
    allRows.append(r);
  };

  auto view = [&](int g, const char* key, const QString& name, const QString& desc, int bit) {
    Row r;
    r.key = QString::fromLatin1(key);
    r.name = name;
    r.description = desc;
    r.group = g;
    r.viewBit = bit;
    allRows.append(r);
  };

  auto overlay = [&](int g, MapEngine::Layer layer) {
    Row r;
    r.key = QStringLiteral("overlay%1").arg(static_cast<int>(layer));
    r.name = MapEngine::layerName(layer);
    r.description = MapEngine::layerDescription(layer);
    r.group = g;
    r.overlayBit = static_cast<int>(layer);
    allRows.append(r);
  };

  // ── Guides ──────────────────────────────────────────────────────────────────
  group(GuidesGroup);
  view(GuidesGroup, "blockGrid", tr("Block grid"),
       tr("The 32-pixel cells a map is really made of. A map is a grid of BLOCKS, never of "
          "single tiles."), ViewBlockGrid);
  view(GuidesGroup, "tileGrid", tr("Tile grid"),
       tr("The 8-pixel tiles inside each block — 16 of them, 4×4. This is a lot of lines; it is "
          "off until you want it."), ViewTileGrid);
  view(GuidesGroup, "mapBounds", tr("Map bounds"),
       tr("Where the real map ends and the 3-block border ring begins."), ViewMapBounds);
  view(GuidesGroup, "connections", tr("Connections"),
       tr("Where each neighbouring map bleeds into the ring, and how big that strip is. The ring is "
          "not a wall of trees — Pallet Town's is really the bottom of Route 1 and the top of Route "
          "21, which is why the walk across is seamless."), ViewConnections);

  // The border ring is an OVERLAY bit (MapEngine paints it), but it belongs here: it is a way of
  // seeing the map's shape, not a fact about its tiles.
  overlay(GuidesGroup, MapEngine::LayerBorder);

  // ── Components ──────────────────────────────────────────────────────────────
  group(ComponentsGroup);
  overlay(ComponentsGroup, MapEngine::LayerWalls);
  overlay(ComponentsGroup, MapEngine::LayerGrass);
  overlay(ComponentsGroup, MapEngine::LayerWater);
  overlay(ComponentsGroup, MapEngine::LayerWarps);
  overlay(ComponentsGroup, MapEngine::LayerDoors);
  overlay(ComponentsGroup, MapEngine::LayerLedges);
  overlay(ComponentsGroup, MapEngine::LayerCounters);
  overlay(ComponentsGroup, MapEngine::LayerCutTrees);
  overlay(ComponentsGroup, MapEngine::LayerElevation);

  // ── Game View ───────────────────────────────────────────────────────────────
  group(GameViewGroup);
  view(GameViewGroup, "player", tr("Player"),
       tr("Him, drawn where the console's own OAM puts him — 4 pixels above his tile row, and "
          "facing right means facing LEFT, mirrored: the game has no right-facing art."),
       ViewPlayer);
  view(GameViewGroup, "npcs", tr("People & objects"),
       tr("Everyone else on this map — the other fifteen sprite slots. Drawn from the game's own "
          "artwork, where the console puts them. A sprite whose picture this map hasn't loaded is "
          "marked: that is the one the game would draw as garbage."),
       ViewNpcs);
  view(GameViewGroup, "screenBox", tr("Screen box"),
       tr("The 20×18 tiles the Game Boy is actually showing — the screen, sliding around inside "
          "the draw area in half-block steps."), ViewScreenBox);
  view(GameViewGroup, "drawArea", tr("Draw area"),
       tr("The 6×5 blocks the game redraws around the player (LoadCurrentMapView). Always "
          "block-aligned."), ViewDrawArea);
}

void MapLayersModel::rebuild()
{
  // `rows` is only what is SHOWN. Every question about STATE is answered from allRows, so a folded
  // group still tells the truth about what it holds.
  beginResetModel();
  rows.clear();

  for (const Row& r : allRows) {
    if (r.isGroup || expanded[r.group])
      rows.append(r);
  }

  endResetModel();
}

// ── One row's state ───────────────────────────────────────────────────────────

bool MapLayersModel::rowVisible(const Row& r) const
{
  if (r.viewBit != 0)
    return (bits & r.viewBit) != 0;

  if (r.overlayBit != 0 && map != nullptr)
    return map->layerOn(r.overlayBit);

  return false;
}

bool MapLayersModel::rowApplies(const Row& r) const
{
  // A map with no neighbours has no strips, and the row should say so rather than switch on nothing.
  // (An indoor map is the common case -- most of the game's maps connect to nobody.)
  if (r.viewBit == ViewConnections)
    return map != nullptr && map->valid() && !map->connectionList().isEmpty();

  // Any other guide always applies -- there is always a grid to draw. A semantic overlay might have
  // nothing to show on THIS map, and if so the row says so rather than switching on an empty
  // overlay and leaving you to wonder whether the feature is broken.
  if (r.overlayBit == 0 || map == nullptr)
    return map != nullptr && map->valid();

  const QVariantList all = map->layerList();
  for (const QVariant& v : all) {
    const QVariantMap m = v.toMap();
    if (m.value("layer").toInt() == r.overlayBit)
      return m.value("applies").toBool();
  }

  return false;
}

void MapLayersModel::setRowVisible(const Row& r, bool on)
{
  if (r.viewBit != 0) {
    const int next = on ? (bits | r.viewBit) : (bits & ~r.viewBit);
    if (next != bits) {
      bits = next;
      emit viewBitsChanged();
    }
    return;
  }

  if (r.overlayBit != 0 && map != nullptr && map->layerOn(r.overlayBit) != on)
    map->toggleLayer(r.overlayBit);
}

// ── The model ─────────────────────────────────────────────────────────────────

int MapLayersModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : rows.size();
}

QVariant MapLayersModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || index.row() < 0 || index.row() >= rows.size())
    return QVariant();

  const Row& r = rows[index.row()];

  switch (role) {
  case NameRole:        return r.name;
  case DescriptionRole: return r.description;
  case IsGroupRole:     return r.isGroup;
  case KeyRole:         return r.key;
  case ExpandedRole:    return r.isGroup ? expanded[r.group] : true;

  case VisibleRole:
    if (!r.isGroup)
      return rowVisible(r);

    // A group with a folded-away child list still has to answer honestly -- so its eye is computed
    // from what it CONTAINS (allRows), never from the rows that happen to be on screen.
    for (const Row& c : allRows)
      if (!c.isGroup && c.group == r.group && rowVisible(c))
        return true;
    return false;

  case GroupStateRole: {
    if (!r.isGroup)
      return 0;

    int on = 0, total = 0;
    for (const Row& c : allRows) {
      if (c.isGroup || c.group != r.group)
        continue;
      total++;
      if (rowVisible(c))
        on++;
    }

    if (total == 0 || on == 0) return 0;   // none
    if (on == total)           return 2;   // all
    return 1;                              // some
  }

  case AppliesRole:  return r.isGroup ? true : rowApplies(r);

  case ColorRole:
    if (r.overlayBit != 0)
      return MapEngine::layerColor(static_cast<MapEngine::Layer>(r.overlayBit));

    // ── The view layers' ink ────────────────────────────────────────────────────────────────
    //
    // These are the EXACT colours MapCanvas draws them in, so the row IS the legend and there is no
    // second source of truth to drift from.
    //
    // Re-picked 2026-07-13 (Twilight: "lots of red outlines, it looks confusing"). The old set was
    // the app's theme colours -- error red, primary pink, accent blue -- which put three warm,
    // similar lines over a grey map and made none of them mean anything. These are Okabe-Ito, the
    // standard colour-blind-safe set: they stay distinct to every kind of colour vision, and they
    // are muted enough to sit over four shades of grey without shouting.
    if (r.viewBit == ViewScreenBox)   return QColor(QStringLiteral("#e69f00")); // orange -- what he SEES
    if (r.viewBit == ViewDrawArea)    return QColor(QStringLiteral("#009e73")); // green  -- what the game REDRAWS
    if (r.viewBit == ViewMapBounds)   return QColor(QStringLiteral("#0072b2")); // blue   -- where the map ENDS
    if (r.viewBit == ViewConnections) return QColor(QStringLiteral("#d55e00")); // vermillion -- the neighbours
    if (r.viewBit == ViewPlayer)      return QColor(QStringLiteral("#6b6b6b")); // him
    if (r.viewBit == ViewNpcs)        return QColor(QStringLiteral("#cc79a7")); // everyone else

    // The grids. ⚠️ OPAQUE here, even though the canvas draws them at low alpha: this colour is what
    // the layer PANEL paints its swatch with, and a translucent swatch on a white row looked
    // greyed-OUT -- i.e. like a layer that was disabled (Twilight, 2026-07-13). The swatch says
    // *which* ink; the canvas decides how strongly to use it.
    return QColor(QStringLiteral("#56b4e9"));                                   // sky blue -- the grids

  default:
    break;
  }

  return QVariant();
}

QHash<int, QByteArray> MapLayersModel::roleNames() const
{
  return {
    { NameRole,        "layerName" },
    { DescriptionRole, "layerDescription" },
    { IsGroupRole,     "layerIsGroup" },
    { VisibleRole,     "layerVisible" },
    { GroupStateRole,  "layerGroupState" },
    { AppliesRole,     "layerApplies" },
    { ColorRole,       "layerColor" },
    { ExpandedRole,    "layerExpanded" },
    { KeyRole,         "layerKey" },
  };
}

// ── The verbs ─────────────────────────────────────────────────────────────────

void MapLayersModel::toggle(int row)
{
  if (row < 0 || row >= rows.size())
    return;

  // Toggling anything by hand ends a solo -- otherwise the "restore" would put back a state the
  // user has since edited, which is worse than not restoring at all.
  soloKey.clear();

  const Row r = rows[row];

  if (!r.isGroup) {
    if (!rowApplies(r) && !rowVisible(r))
      return;   // nothing to show: the row says so, and the click does nothing rather than lie

    setRowVisible(r, !rowVisible(r));
    refreshAll();
    return;
  }

  // A group's eye: if ANY child is on, one click turns the group OFF; otherwise it turns on every
  // child that has something to show. One click always changes something -- which is the whole
  // contract of a tri-state eye. Over allRows, so a FOLDED group still toggles everything it holds.
  bool any = false;
  for (const Row& c : allRows)
    if (!c.isGroup && c.group == r.group && rowVisible(c)) { any = true; break; }

  for (const Row& c : allRows) {
    if (c.isGroup || c.group != r.group)
      continue;

    if (any)
      setRowVisible(c, false);
    else if (rowApplies(c))
      setRowVisible(c, true);
  }

  refreshAll();
}

void MapLayersModel::solo(int row)
{
  if (row < 0 || row >= rows.size() || rows[row].isGroup)
    return;

  const Row r = rows[row];

  // Soloing the soloed row again puts back exactly what was on before. A solo is a LOOK, not a
  // destructive edit of the setup you spent time getting right.
  if (soloKey == r.key) {
    soloKey.clear();

    if (bits != savedView) {
      bits = savedView;
      emit viewBitsChanged();
    }

    if (map != nullptr && map->layers() != savedOverlay)
      map->setLayers(savedOverlay);

    refreshAll();
    return;
  }

  if (soloKey.isEmpty()) {
    savedView = bits;
    savedOverlay = (map != nullptr) ? map->layers() : 0;
  }

  soloKey = r.key;

  // Everything in this group off except the one; every other group left exactly as it is (soloing
  // "Grass" should not switch off the map bounds you are using to read the map).
  for (const Row& c : allRows) {
    if (c.isGroup || c.group != r.group)
      continue;

    setRowVisible(c, c.key == r.key);
  }

  refreshAll();
}

void MapLayersModel::toggleExpanded(int row)
{
  if (row < 0 || row >= rows.size() || !rows[row].isGroup)
    return;

  expanded[rows[row].group] = !expanded[rows[row].group];
  rebuild();
}

void MapLayersModel::clearAll()
{
  soloKey.clear();

  if (bits != ViewNone) {
    bits = ViewNone;
    emit viewBitsChanged();
  }

  if (map != nullptr)
    map->setLayers(0);

  refreshAll();
}

void MapLayersModel::clearGroup(int row)
{
  if (row < 0 || row >= rows.size())
    return;

  const int g = rows[row].group;

  // A solo is a LOOK on top of a setup. Clearing the group it was in throws both away -- keeping the
  // snapshot would mean a later alt-click restored layers the user had explicitly just cleared.
  if (!soloKey.isEmpty()) {
    for (const Row& c : allRows) {
      if (!c.isGroup && c.group == g && c.key == soloKey) {
        soloKey.clear();
        break;
      }
    }
  }

  for (const Row& c : allRows) {
    if (c.isGroup || c.group != g)
      continue;

    setRowVisible(c, false);
  }

  refreshAll();
}

bool MapLayersModel::anyOn() const
{
  return bits != ViewNone || (map != nullptr && map->layers() != 0);
}

bool MapLayersModel::groupAnyOn(int row) const
{
  if (row < 0 || row >= rows.size())
    return false;

  const int g = rows[row].group;

  for (const Row& c : allRows) {
    if (!c.isGroup && c.group == g && rowVisible(c))
      return true;
  }

  return false;
}

void MapLayersModel::refreshAll()
{
  if (rows.isEmpty())
    return;

  emit dataChanged(index(0, 0), index(rows.size() - 1, 0));
}

// ── The bits QML binds to ─────────────────────────────────────────────────────

int MapLayersModel::viewBits() const     { return bits; }
bool MapLayersModel::showBlockGrid() const { return (bits & ViewBlockGrid) != 0; }
bool MapLayersModel::showTileGrid() const  { return (bits & ViewTileGrid) != 0; }
bool MapLayersModel::showMapBounds() const { return (bits & ViewMapBounds) != 0; }
bool MapLayersModel::showPlayer() const    { return (bits & ViewPlayer) != 0; }
bool MapLayersModel::showScreenBox() const { return (bits & ViewScreenBox) != 0; }
bool MapLayersModel::showDrawArea() const  { return (bits & ViewDrawArea) != 0; }
bool MapLayersModel::showConnections() const { return (bits & ViewConnections) != 0; }
bool MapLayersModel::showNpcs() const       { return (bits & ViewNpcs) != 0; }

qreal MapLayersModel::overlayOpacity() const { return opacity; }

void MapLayersModel::setOverlayOpacity(qreal o)
{
  o = qBound(0.15, o, 1.0);   // 0 would be "on, but invisible" -- a switch that lies about itself
  if (qFuzzyCompare(opacity, o))
    return;

  opacity = o;
  emit overlayOpacityChanged();
}
