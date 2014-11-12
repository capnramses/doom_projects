doom_port
=========

my port of Id Software's DOOM

i wrote a blog post about this http://antongerdelan.net/blog/?post=2014_09_24

at the moment it runs on 32-bit and 64-bit linux machines, but should be pretty
easy to port to windows and apple, i just need to create Makefiles - required
libs are here already

#what i haven't done yet#

* Finish the keyboard controls
* Recreate the sound system in a multi-platform way (OpenAL might be okay -
FMOD or IrrKlang better)
* Improve the efficiency of frame-drawing (who cares, it only has to run at
35Hz)
* Fix a couple of crash-bugs in the original source (one of the "are you sure
you want to quit?" messages is stack smashing) - there are some pretty naughty
hidden quit messages in there btw.
* Fiddle with the original code - weapons, baddies, etc.

