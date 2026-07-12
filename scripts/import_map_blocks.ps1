# import_map_blocks.ps1 -- import the map block layouts (.blk) and tileset blocksets
# (.bst) from the pret/pokered disassembly into the db resources, verbatim.
#
# Source of truth: assets/references/pokered (the read-only reference clone).
#   maps/<Label>.blk            one byte per block, width*height bytes, row-major
#   gfx/blocksets/<name>.bst    one block per 16 bytes (4x4 tile ids, row-major)
#
# Output (byte-identical content, renamed to the id the editor looks it up by):
#   projects/db/assets/blocks/maps/<mapId>.blk
#   projects/db/assets/blocks/tilesets/<tilesetId>.bst
#
# Ids are the ids the SAVE stores (wCurMap 0x260A / wCurMapTileset) and the same
# `ind` values maps.json / tileset.json already use, so the editor never needs a
# name lookup or a manifest -- just the id.
#
# The mapping is taken from the disassembly, never guessed from file names:
#   data/maps/map_header_pointers.asm  id      -> header label   (authoritative)
#   data/maps/headers/<Label>.asm      label   -> map const + tileset const
#   constants/map_constants.asm        const   -> id, width, height
#   constants/tileset_constants.asm    const   -> tileset id
#   maps.asm                           <Label>_Blocks -> the INCBIN'd .blk
# That last one matters: several maps share one .blk through label aliases
# (UndergroundPathRoute5/6/7/7Copy all point at UndergroundPathRoute5.blk), and
# UndergroundPathRoute7Copy has no .blk of its own at all.
#
# ROM OVERRUN (faithfully reproduced, not patched): UNDERGROUND_PATH_NORTH_SOUTH's
# header declares 4x24 = 96 blocks but its .blk is only 92 bytes -- the disassembly
# says so out loud ("UndergroundPathNorthSouth.blk is actually 4x23"). The game
# copies width*height bytes regardless, so its last row is really the first 4 bytes
# of whatever ROM follows -- UndergroundPathWestEast_Blocks, the next INCBIN. We
# reproduce that by reading on through the following blocks in maps.asm order,
# exactly as the Game Boy does. The script reports every map this happens to.
#
# VALIDATION (refuses to write anything that does not line up):
#   * every map's block data is exactly width*height bytes (after any ROM overrun)
#   * those dimensions agree with maps.json
#   * every block id a map uses exists in that map's tileset blockset
#
# Run:  pwsh -File scripts/import_map_blocks.ps1  [-Check]
#   -Check  validate only, write nothing.

[CmdletBinding()]
param(
  [string]$Pokered = "$PSScriptRoot\..\assets\references\pokered",
  [string]$OutRoot = "$PSScriptRoot\..\projects\db\assets\blocks",
  [switch]$Check
)

$ErrorActionPreference = "Stop"
$repo = Resolve-Path "$PSScriptRoot\.."
$Pokered = Resolve-Path $Pokered

# --- tileset id -> blockset file -------------------------------------------------
# From gfx/tilesets.asm: several tilesets are aliases sharing one blockset
# (RedsHouse1/2, Dojo/Gym, Mart/Pokecenter, ForestGate/Museum/Gate). We write one
# .bst per tileset id so lookup is always "by the id the save stores"; the shared
# ones are byte-identical copies, exactly as the app's tileset PNGs already are.
$tilesetBlockset = @(
  'overworld',    # 0  OVERWORLD
  'reds_house',   # 1  REDS_HOUSE_1
  'pokecenter',   # 2  MART
  'forest',       # 3  FOREST
  'reds_house',   # 4  REDS_HOUSE_2
  'gym',          # 5  DOJO
  'pokecenter',   # 6  POKECENTER
  'gym',          # 7  GYM
  'house',        # 8  HOUSE
  'gate',         # 9  FOREST_GATE
  'gate',         # 10 MUSEUM
  'underground',  # 11 UNDERGROUND
  'gate',         # 12 GATE
  'ship',         # 13 SHIP
  'ship_port',    # 14 SHIP_PORT
  'cemetery',     # 15 CEMETERY
  'interior',     # 16 INTERIOR
  'cavern',       # 17 CAVERN
  'lobby',        # 18 LOBBY
  'mansion',      # 19 MANSION
  'lab',          # 20 LAB
  'club',         # 21 CLUB
  'facility',     # 22 FACILITY
  'plateau'       # 23 PLATEAU
)

# --- constants/map_constants.asm -> const -> id, width, height --------------------
$mapConst = @{}
$id = 0
foreach ($line in Get-Content "$Pokered\constants\map_constants.asm") {
  if ($line -match '^\s*map_const\s+(\w+)\s*,\s*(\d+)\s*,\s*(\d+)') {
    $mapConst[$Matches[1]] = @{ id = $id; width = [int]$Matches[2]; height = [int]$Matches[3] }
    $id++
  }
  elseif ($line -match '^\s*const\s+(\w+)') {
    $mapConst[$Matches[1]] = @{ id = $id; width = 0; height = 0 }
    $id++
  }
}

# --- constants/tileset_constants.asm -> const -> id -------------------------------
$tilesetConst = @{}
$tid = 0
foreach ($line in Get-Content "$Pokered\constants\tileset_constants.asm") {
  if ($line -match '^\s*const\s+(\w+)') { $tilesetConst[$Matches[1]] = $tid; $tid++ }
}

# --- data/maps/map_header_pointers.asm -> map id -> header label ------------------
# 248 entries, one per map id. The unused/"copy" ids are not blank -- they point at a
# real map's header (UNUSED_MAP_0B -> SaffronCity_h, UNUSED_MAP_69 -> LancesRoom_h,
# ...), so that is genuinely what the game would load for them, and we import them
# the same way. Their entries carry a trailing comment, which is why the pattern
# must allow one -- dropping them silently shifts every later id by one.
$idToLabel = @{}
$i = 0
foreach ($line in Get-Content "$Pokered\data\maps\map_header_pointers.asm") {
  if ($line -match '^\s*dw\s+(\w+)_h\s*(;.*)?$') { $idToLabel[$i] = $Matches[1]; $i++ }
}

# --- data/maps/headers/<Label>.asm -> label -> map const + tileset const -----------
$headerOf = @{}
foreach ($f in Get-ChildItem "$Pokered\data\maps\headers\*.asm") {
  $txt = Get-Content $f.FullName -Raw
  if ($txt -match 'map_header\s+(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,') {
    $headerOf[$Matches[1]] = @{ mapConst = $Matches[2]; tileset = $Matches[3] }
  }
}

# --- maps.asm -> <Label>_Blocks -> .blk file, in ROM order -------------------------
# Consecutive bare labels alias the INCBIN that follows them.
$blocksOf   = @{}   # label -> blk file name (no extension)
$romOrder   = @()   # blk file names in the order they appear (for the overrun)
$pending    = @()
foreach ($line in Get-Content "$Pokered\maps.asm") {
  if ($line -match '^(\w+)_Blocks:\s*INCBIN\s+"maps/([\w]+)\.blk"') {
    $pending += $Matches[1]
    foreach ($lbl in $pending) { $blocksOf[$lbl] = $Matches[2] }
    $romOrder += $Matches[2]
    $pending = @()
  }
  elseif ($line -match '^(\w+)_Blocks:\s*$') {
    $pending += $Matches[1]
  }
}

$blkBytes = @{}
foreach ($n in ($romOrder | Select-Object -Unique)) {
  $blkBytes[$n] = [System.IO.File]::ReadAllBytes("$Pokered\maps\$n.blk")
}

# Read `count` bytes starting at the beginning of `name`, running on into the blocks
# that follow it in ROM if it is too short -- what the Game Boy actually does.
function Get-MapBlocks([string]$name, [int]$count) {
  $out = New-Object System.Collections.Generic.List[byte]
  $start = $romOrder.IndexOf($name)
  for ($k = $start; $k -lt $romOrder.Count -and $out.Count -lt $count; $k++) {
    $out.AddRange($blkBytes[$romOrder[$k]])
  }
  if ($out.Count -lt $count) { return $null }
  return $out.GetRange(0, $count).ToArray()
}

# --- maps.json (cross-check only; never written to) --------------------------------
$mapsJson = Get-Content "$repo\projects\db\assets\data\maps.json" -Raw | ConvertFrom-Json
$jsonByInd = @{}
foreach ($m in $mapsJson) { if ($null -ne $m.ind) { $jsonByInd[[int]$m.ind] = $m } }

$bst = @{}
foreach ($name in ($tilesetBlockset | Select-Object -Unique)) {
  $bst[$name] = [System.IO.File]::ReadAllBytes("$Pokered\gfx\blocksets\$name.bst")
}

# --- build + validate ---------------------------------------------------------------
$problems = @()
$overruns = @()
$unsized  = @()
$plan     = @()

foreach ($mapId in ($idToLabel.Keys | Sort-Object)) {
  $label = $idToLabel[$mapId]
  if (-not $headerOf.ContainsKey($label))   { $problems += "id $mapId ($label): no header"; continue }
  if (-not $blocksOf.ContainsKey($label))   { $problems += "id $mapId ($label): no _Blocks label in maps.asm"; continue }

  $h  = $headerOf[$label]
  if (-not $mapConst.ContainsKey($h.mapConst))     { $problems += "id $mapId ($label): unknown map const $($h.mapConst)"; continue }
  if (-not $tilesetConst.ContainsKey($h.tileset))  { $problems += "id $mapId ($label): unknown tileset $($h.tileset)"; continue }

  $mc   = $mapConst[$h.mapConst]
  $tsId = $tilesetConst[$h.tileset]
  $w    = $mc.width
  $hgt  = $mc.height
  $need = $w * $hgt

  $src   = $blocksOf[$label]
  $raw   = $blkBytes[$src]
  $bytes = Get-MapBlocks $src $need
  if ($null -eq $bytes) { $problems += "id $mapId ($label): cannot fill $need bytes from $src.blk"; continue }
  if ($raw.Length -lt $need) {
    $overruns += "id $mapId ($label): $src.blk is $($raw.Length) bytes but the header says ${w}x${hgt} = $need -- $($need - $raw.Length) byte(s) read on into the next blocks in ROM (as the game does)"
  }

  # Dimensions must agree with maps.json.
  #
  # The 22 glitch ids ("Unused Map XX") carry NO width/height there: maps.json treats
  # them as unused, while the ROM's header table quietly points them at a real map's
  # header (id 11 -> Saffron City, 105-117 -> Lance's Room, ...). We do NOT import
  # block data we cannot legitimately size -- the editor's DB is the thing that has to
  # describe a map, and it says these have no dimensions. Skipped, and reported.
  $j = $jsonByInd[[int]$mapId]
  if ($null -eq $j) { $problems += "id $mapId ($label): no maps.json entry"; continue }
  if ($null -eq $j.width -or "" -eq [string]$j.width) {
    $unsized += "id $mapId (maps.json: '$($j.name)', glitch) -- ROM would load $label's header (${w}x${hgt})"
    continue
  }
  if ([int]$j.width -ne $w -or [int]$j.height -ne $hgt) {
    $problems += "id $mapId ($label): maps.json says $($j.width)x$($j.height), disassembly says ${w}x${hgt}"
  }

  # every block id used must exist in the tileset's blockset
  $blockCount = $bst[$tilesetBlockset[$tsId]].Length / 16
  $maxUsed = ($bytes | Measure-Object -Maximum).Maximum
  if ($maxUsed -ge $blockCount) {
    $problems += "id $mapId ($label): uses block $maxUsed but tileset $($h.tileset) has only $blockCount blocks"
  }

  $plan += [pscustomobject]@{
    Id = $mapId; Label = $label; Src = $src; Bytes = $bytes
    Tileset = $tsId; TilesetName = $h.tileset; W = $w; H = $hgt
  }
}

Write-Host "maps      : $($plan.Count) imported (ids 0..$(($plan.Id | Measure-Object -Maximum).Maximum))"
Write-Host "tilesets  : $($tilesetBlockset.Count)"
if ($unsized.Count) {
  Write-Host "`nskipped -- maps.json gives these no size ($($unsized.Count)):" -ForegroundColor Yellow
  $unsized | ForEach-Object { Write-Host "  $_" -ForegroundColor Yellow }
}
if ($overruns.Count) {
  Write-Host "`nROM overruns reproduced ($($overruns.Count)):" -ForegroundColor Yellow
  $overruns | ForEach-Object { Write-Host "  $_" -ForegroundColor Yellow }
}
if ($problems.Count) {
  Write-Host "`nPROBLEMS ($($problems.Count)):" -ForegroundColor Red
  $problems | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
  throw "import aborted -- source data did not validate"
}
Write-Host "`nvalidation passed" -ForegroundColor Green

if ($Check) { Write-Host "-Check: nothing written."; return }

# --- write ----------------------------------------------------------------------------
$mapOut = Join-Path $OutRoot "maps"
$tsOut  = Join-Path $OutRoot "tilesets"
New-Item -ItemType Directory -Force -Path $mapOut, $tsOut | Out-Null
Get-ChildItem $mapOut, $tsOut -File -ErrorAction SilentlyContinue | Remove-Item -Force

foreach ($p in $plan) { [System.IO.File]::WriteAllBytes((Join-Path $mapOut "$($p.Id).blk"), $p.Bytes) }
for ($k = 0; $k -lt $tilesetBlockset.Count; $k++) {
  [System.IO.File]::WriteAllBytes((Join-Path $tsOut "$k.bst"), $bst[$tilesetBlockset[$k]])
}

# --- qrc fragment ----------------------------------------------------------------------
# A build artifact, not repo content: paste it into projects/db/db.qrc when the set of
# imported maps changes. Written to tmp/ (git-ignored) so it never lands in the assets.
$tmp = Join-Path $repo "tmp"
New-Item -ItemType Directory -Force -Path $tmp | Out-Null

$qrc = @()
foreach ($p in ($plan | Sort-Object Id)) { $qrc += "        <file>assets/blocks/maps/$($p.Id).blk</file>" }
for ($k = 0; $k -lt $tilesetBlockset.Count; $k++) { $qrc += "        <file>assets/blocks/tilesets/$k.bst</file>" }
($qrc -join "`n") | Set-Content "$tmp\blocks-qrc-fragment.txt" -Encoding utf8
Write-Host "qrc fragment: $tmp\blocks-qrc-fragment.txt (paste into projects/db/db.qrc)"

# --- provenance -------------------------------------------------------------------------
$md = @()
$md += "# Map block data"
$md += ""
$md += "Imported **verbatim** from the pret/pokered disassembly by ``scripts/import_map_blocks.ps1``."
$md += "Do not hand-edit -- re-run the script (``-Check`` validates without writing). It re-derives"
$md += "and re-validates every byte on each run."
$md += ""
$md += "* ``maps/<mapId>.blk`` -- one byte per block, ``width * height`` bytes, row-major."
$md += "  ``<mapId>`` is the id the save stores in ``wCurMap`` (0x260A) and the ``ind`` in ``maps.json``."
$md += "* ``tilesets/<tilesetId>.bst`` -- one block per 16 bytes (4x4 tile ids, row-major)."
$md += "  ``<tilesetId>`` is the id in ``wCurMapTileset`` and the ``ind`` in ``tileset.json``."
$md += "  Alias tilesets share a blockset, so several of these files are byte-identical copies."
$md += ""
$md += "Ids come from ``data/maps/map_header_pointers.asm`` (the authoritative id -> header table),"
$md += "not from file names: several maps share one ``.blk`` through label aliases, and"
$md += "``UndergroundPathRoute7Copy`` has no ``.blk`` of its own."
$md += ""
if ($unsized.Count) {
  $md += "## Glitch ids: imported, or not"
  $md += ""
  $md += "``maps.json`` marks $($unsized.Count) ids as unused glitch maps and gives them **no width or"
  $md += "height**. The ROM is less tidy: its header table quietly points those ids at a real map's"
  $md += "header, so a Game Boy would happily render one. We do not import them -- a map the editor's"
  $md += "own DB cannot size is a map it has no business drawing, and inventing the dimensions is not"
  $md += "ours to do. The map screen says so plainly instead."
  $md += ""
  foreach ($u in $unsized) { $md += "* $u" }
  $md += ""
  $md += "(The reachable ``*_COPY`` ids are a different thing -- ``maps.json`` sizes those, and they import"
  $md += "normally.)"
  $md += ""
}
if ($overruns.Count) {
  $md += "## ROM overruns (reproduced, not patched)"
  $md += ""
  $md += "The game copies ``width * height`` block bytes no matter how long the ``.blk`` actually is,"
  $md += "so a short one runs on into the next map's blocks in ROM. That is real, shipped behaviour"
  $md += "and it is what these files contain:"
  $md += ""
  foreach ($o in $overruns) { $md += "* $o" }
  $md += ""
}
$md += "## Tilesets"
$md += ""
$md += "| id | tileset | blockset | blocks |"
$md += "|----|---------|----------|--------|"
foreach ($k in ($tilesetConst.Keys | Sort-Object { $tilesetConst[$_] })) {
  $n = $tilesetConst[$k]
  $md += "| $n | $k | ``$($tilesetBlockset[$n]).bst`` | $($bst[$tilesetBlockset[$n]].Length / 16) |"
}
$md += ""
$md += "## Maps"
$md += ""
$md += "| id | map | tileset | source | blocks (w x h) |"
$md += "|----|-----|---------|--------|----------------|"
foreach ($p in ($plan | Sort-Object Id)) {
  $nm = if ($jsonByInd[[int]$p.Id]) { $jsonByInd[[int]$p.Id].name } else { $p.Label }
  $md += "| $($p.Id) | $nm | $($p.TilesetName) | ``maps/$($p.Src).blk`` | $($p.W) x $($p.H) |"
}
($md -join "`n") | Set-Content "$OutRoot\README.md" -Encoding utf8

Write-Host "wrote $($plan.Count) .blk + $($tilesetBlockset.Count) .bst to $OutRoot"
