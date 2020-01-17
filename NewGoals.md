# New goals for the reboot

The reboot plans to shed hundreds of layers of complexity, keep a beautiful
and responsive UI/UX, and bring the project into C++ with it's speed and 
power. However the project is doing that so it can go above and beyond it's
limits when it was within all the Javascript/Electron layers.

## Smarter Data Handling

So previosuly, the app was aware of save file data, presented it to you, and 
allowed you to simply change it. That's really great for a "Save File Editor"
and follows the KISS principle (Keep it Stupid Simple). A great program is one
that isn't too complicated and doesn't try to do too much.

However another feature of KISS is one that allows the user to go into a
simple interface that presents everything very simply allowing it all to be 
changed. An interface that's intuitive, laid out well, not complicated or 
bloated, doesn't requires much of nay explanations, and makes things overall
very simple to play around with and have fun editing.

To do that the program needs to do more than present raw data, it needs to
understand the data it's presenting. The more it understands the data, the more
it can offer the user ways to have fun with and play around with a save file
without complicating anything.

## Map Creation

So a really big part of the save file is the area that you're in. It actually
contains hundreds of variables and many of them are quite complicated. Following
the KISS principle I want the user to be able to change the map they're on to
a different map and even be encouraged to go out and have fun with the map.

Did you know you can change connecting maps, warps, sprites, positions, music,
even Pokemon. There is so much of the save file dedicated to the map your own.
A really big focus of this reboot is to, without complicating anything, allow
you to do so much more than possible before with the current area your own.

What I'm imagining the UX to be like is to be able to go to any map in the
game, click a load button to load your character there onto the map, and be
able to go all out editing the map with editing options you probably didn't
even know about. The program will help you every step of the way by doing the
grunt of setting everything up like idefault warps, sprites, connecting data,
and handle pointer calculations seamlessly in the background while making it 
available to you still if you want full power.

## Randomization

Here's another really exciting feature, Imagine you want to play a new game but
you want a random name, a random team, to be on a random map, given random items,
random money, everythign to be random. A SAV file where you just don't have any 
idea what your walking into and that's the fun of it. Of course you can further
edit the randomized save file and either re-roll certain randomizations or just
re-roll the entire save file over again.

This has been a big feature since the dawn of the reboot because it's so 
exciting. Very early testing has shown that, even though it's quite buggy still
it was pretty exciting to be dropped into Pokemon Tower as Paddles with a ghost
Pikachu named Wolvoom that knows the wildest of moves and navigating out of
the Pokemon Tower to begin my Pokemon Adventures only to encounter a level 5
Zapdos along the way and the whole map being darker than normal.

So much potential with this feature that's still in the early in-dev phase of
development.

### More on Randomization

The goal of randomization mentioned above is to specifically place you into
a highly randomized sav file. But if we just go away randomizing everything
without thought the game will almost 100% of the time not be playable and will
liekly crash. If that were the case the feature would never be used.

Instead Randomization focuses on these goals:

* To provide a totally random sav file
* To have you start your adventure, all randomized content will be geared to
  starting your adventure. I still want you to play through the game like a new
  game.
* To not provide you too much of stuff, like too many crazy items, insane
  amounts of money, insane Pokemon, etc...
* To not have any glitch items, pokemon, or related
* To have an "escape" pokemon in your team, a random HM slave with the ability
  to use them out of battle. I don't have any idea where you'll end up.
* To have a random "starter" pack of sorts with potentials for rare items at
  start of game.
* To be very careful in arranging things to not crash anything
* To have no play progress, no trainers beaten, no events completed, nothing.
  This is a new save file your starting with, that's the goal.
* To above all, have fun. You should be laughing as you play the save file and
  have this curious wonder what you'll run into.

The current stage is extremely buggy but even with this incredibly early feature
it was so fun to play I found myself re-rolling new save files even throughout
the bugs. I can't wait to begin smoothing out and polishing this feature.

## More features and goals to come

I'll add more goals and such here as I take short breaks to write them. There's
much more planned!
