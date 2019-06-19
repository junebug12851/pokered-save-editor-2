# Pokered Save Editor 2
Pokemon Red & Blue Save Editor 2 ~ Qt C++ Reboot

## An early WIP

Please See https://github.com/junebug12851/pokered-save-editor which contains a much more mature version 
until this leaves WIP status.

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
