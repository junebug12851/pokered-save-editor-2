# Credits {#credits}

Everyone and everything that helped build Pokered Save Editor 2 -- contributors, data sources, frameworks, tools, services, and the artists behind the icons and artwork.

> This page is the human-readable rendering of `projects/db/assets/data/credits.json` (the data behind the in-app Credits screen). **Regenerate it whenever that JSON changes** -- see `CLAUDE.md` -> "Keep the Credits Screen Living".

## Project Leaders

- **[Twilight](https://github.com/junebug12851)**

## Data Sources

- **[The Pokered Team](https://github.com/pret/pokered)** -- None of this would have been possible without them

## Framework

- **[Qt Open Source Edition](https://qt.io)** -- Literally powers this app _(License: GNU Lesser General Public License (LGPL) version 3)_

## AI Assistance

- **[Claude (Anthropic)](https://claude.ai)** -- Helped revive and modernize the project in 2026 -- debugging, testing, and the living project notes
- **[ChatGPT (OpenAI)](https://chatgpt.com)** -- Development and design help, including the in-app status symbol icons, the gym-badge images, the gym leader portraits, and the rival artwork

## Tools Used

- **[PyBoy](https://github.com/Baekalfen/PyBoy)** -- A Game Boy emulator we drive from the test suite: it boots the real game with our save files so the editor can be checked against the console itself, byte for byte. A developer tool only: installed separately, run as its own process, and never linked into or shipped with the editor. _(License: LGPL-3.0 -- a dev/test tool, NOT distributed with this app)_
- **[JSON Editor Online](https://jsoneditoronline.org)** -- Huge help in formatting and validating json data
- **[Regular Expressions 101](https://regex101.com)** -- Major help in converting data from the Pokered Team to JSON
- **[Codepen](https://codepen.io)** -- Huge help in further converting data from the Pokered Team to JSON
- **[Quicktype](https://app.quicktype.io)** -- They helped breakdown JSON structure into C++ so much easier!!!
- **[Git](https://git-scm.com)** -- Powers the projects VCS
- **[Docker](https://docker.com)** -- Containerized Linux build/test environment -- runs the full test suite plus AddressSanitizer/UBSan and coverage, which can't run on the Windows kit
- **[Pillow](https://python-pillow.github.io)** -- Assembles UI screenshot frames into animated GIFs (scripts/make_gifs.py) -- used manually to add GIFs; the automated screenshot pipeline itself produces still PNGs only _(License: MIT-CMU)_
- **[Inno Setup](https://jrsoftware.org/isinfo.php)** -- Builds the Windows installer in the release pipeline
- **[linuxdeploy](https://github.com/linuxdeploy/linuxdeploy)** -- Bundles the Linux AppImage (with its Qt plugin) in the release pipeline

## Services Used

- **[Github](https://github.com)** -- Gracously host so many projects free including this one.

## Icons

- **[Android Asset Studio](https://romannurik.github.io/AndroidAssetStudio/icons-launcher.html)** -- The program icon _(License: Apache 2)_
- **[Google Material Icons](https://material.io/resources/icons)** -- The program icon _(License: Apache 2)_
- **[Font Awesome Free](https://fontawesome.com/icons)** -- Most of the non-game specific icons! Huge shoutout to them. _(License: Creative Commons Attribution 4.0)_
- **[Design Revision](https://www.iconfinder.com/DesignRevision)** -- Some of the other non-game specific icons _(License: Custom License)_
- **[Greepit](https://greepit.com)** -- Some of the other non-game specific icons _(License: Custom License)_
- **[Roundicons Freebies](https://flaticon.com/packs/pokemon-go)** -- Pokemon Go Icons used on home page!!! _(License: Custom License)_
  - _Required attribution: Icon made by Roundicons Freebies from www.flaticon.com_
- **[Berkah Icon](https://iconfinder.com/icons/1320027/go_hat_pokemon_icon)** -- Some other home screen Pokemon icons _(License: Creative Commons Attribution 2.5 Generic (CC BY 2.5))_
- **[TheArtificial Pokemon Icons Team](https://github.com/TheArtificial/pokemon-icons)** -- Literally all 151 Pokemon icons including shinies, Huge shoutout to the amazing team! _(License: Creative Commons Attribution)_
- **[ChatGPT (OpenAI)](https://chatgpt.com)** -- The in-app status symbol icons, the gym-badge images, the gym leader portraits, and the rival artwork

## Wallpapers

- **[Poke Walk Kanto](https://deviantart.com/ry-spirit/art/Poke-Walk-Kanto-591588328)** -- Featured on New File Page _(License: Creative Commons Attribution, Non-Commercial, No Derivatives 3.0)_
- **[Who wants some Jelly Donuts](https://deviantart.com/ry-spirit/art/Who-wants-some-Jelly-Donuts-373934999)** -- Featured on File Tools Page _(License: Creative Commons Attribution, Non-Commercial, No Derivatives 3.0)_
- **[Basic Pokemons Colors](https://deviantart.com/yoshiyaki/art/Basic-Pokemons-Colors-574585879)** -- Featured here in the Credits _(License: Creative Commons Attribution, Non-Commercial, No Derivatives 3.0)_

