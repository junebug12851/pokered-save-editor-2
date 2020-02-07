# Pokered Save Editor 2
Pokemon Red & Blue Save Editor 2 ~ Qt C++ Reboot

We've reached 300 Commits ٩(๑˃̵ᴗ˂̵)و !

## An early WIP

Please See https://github.com/junebug12851/pokered-save-editor which contains a much more mature version 
until this leaves WIP status.

So this is going to look like nothing to you, a simple image, however it took very close to 300 commits to get to this point. This is the result of QML communicating with the C++ back-end in conjuction with the C++ Pokered Database to extract the player name of the save file and display it.

![initialImage](https://i.imgur.com/LB7LDxr.png)

Looking good so far, more progress!!!

![More Progress in the ui/ux](https://i.imgur.com/zp8xCDc.png)
![3 Stage Name Editor](https://i.imgur.com/kFLN4R2.png)
![Initial New File Screen](https://i.imgur.com/Woakr9P.png)
![Initial File Tools Screen](https://i.imgur.com/4oPvOE9.png)

Here's sort of a rough preview of a before and after, note that the after image below may become outdated as new updates roll in. You can clearly see how beautiful the new interface is.
![Before and After Image](https://i.imgur.com/LS1XPsy.png)

## New page for reboot goals / features

Please see NewGoals.md for the breakdown of new goals and features specific to 
this reboot.

### Large refactoring

So QML is a learning expirience for me, I know Qt and C++ and I love both. While I've always wanted to make a QML project, this is my first and so it's been a pretty big challenge. I initially did a lot of stuff wrong and I could tell that more and more as I progressed, eventually QML sort of forced my hand into figuring out where I went wrong... and re-writing a lot of code leading to a big refactor. But thanks to me doing that everything is much nicer and cleaner, smoother, more expandable in the future, etc...

Sadly one of the things that had to go was the angled bars for the header and footer. I didn't want to let them go but after weighing the prs and cons it would have made expandability and usability a nightmare. It's been very refreshing to use the newer setup but I will mose those really cool angled bars.

There all kinds of other little changes but overall it feels more dynamic and fluid and that's kidn of how I wanted it to feel.

## Why the Reboot

I didn't realize how really messy and overly complicated building a desktop app in pure Javascript
would be. Sure Javascript itself is very simple, I absolutely love Javascript and love to use it
in all kinds of ways from simple to complex but lately it's getting really old to handle so much complexity
and baggage that comes from a large app, especially a desktop app.

I'm havign to juggle Angular, electron, back-end, front-end (because of sandboxing), myriad of
libraries, async and sync coding, overly complicated build systems, complicated debugging from all
this, resources and assets from all sides. It's really just a lot, not really difficult, just.... 
a lot.

Plus electron building in and of itself is complicated and extremely slow. Packages help and
I do use them but they add that much more to worry about and I've already run itno countless problems
with them over the time I've used them.

It takes abut 45 minutes for each and every build if memory serves and the resulting files are gigantic.
It's a cluster mess to put it politely.

On a side note Angular is just too complicated for my needs and is designed for a website, I've run into
far too many issues, glitches, and overall limitations trying to work with and use Angular especially
for an electron desktop app.

For months I've been researching moving back to simpler times with C++ and without losing any of the 
visual appeal of the current UX. Doing this would allow me to remove hundreds of layers, frameworks,
libraries, and overall baggage.

The problem with Qt is that the file size will still be very large and sometimes packaging and deploying
is complicated and error-prone on the other side. However the result is a still beautiful UI/UX with
my toolset being amazingly simpler and built for exactly what I want to do. It also opens up a vast
range of additional features I want to be able to include but haven't because of, well, Angular and
many other complexities.

Furthermore while the build size will still be large, it'll build SUPER QUICK, like REALLY REALLY quick,
like not 45 minutes lol and so so much simpler.

Right now all a WIP but I'm really holding out hope I can make the reboot happen and completed.
