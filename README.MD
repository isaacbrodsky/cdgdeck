# CD+G Deck

CD+G Deck is an application for playing Compact Disc Graphics. In the later 80's, the CD+G format was introduced to embed low bitrate graphics tracks in audio CDs, using the discs' "subcode" fields. A limited selection of CDs have these elaborate but low bitrate graphics tracks embedded in them. You can learn more about CD+G at the [CD+G Museum](http://www.cdplusg.com/cdplusg/Main.html).

This happened contemporaneously with CD Video (not to be confused with Video CD), and a few years before the Philips CD-i. After this initial commercial push, CD+G retreated from public view with the exception of karaoke CDs, which have made continued use of the format. There was also a CD+EG (Compact Disc Extended Graphics) format, but I am not aware of any discs using this standard ever being made. The Sega Saturn may have a reference decoder for the CD+EG format.

This application is focused on playing subcode tracks (doesn't support reading the CD directly) of discs from the CD+G launch. These older discs use more of the features of the CD+G system, including features rarely seen on karaoke CDs, and have the most interesting graphics.

# Building

Install [vcpkg](github.com/microsoft/vcpkg/) for dependency management. You must also need CMake and a C++ compiler.

## vcpkg setup instructions

Install the following:

```
sdl2 sdl2-mixer sdl2-ttf libpng zlib fltk
```

Note, fltk may need static linking.

Optional decoders:

```
sdl2-mixer[libflac] sdl2-mixer[mpg123] sdl2-mixer[libvorbis] --recurse
```

## Build

Replace `$VCPKG_ROOT` with where you installed vcpkg.

```
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake ..
make
```

# History

This code was originally written around 2012-2013. It was mainly tested against *Information Society* (the disc I originally learned about CD+G from), and then later against Jimi Hendrix Experience' *Smash Hits* (probably one of the most technically and 
artistically developed CD+G discs.) I tested against a few karaoke CDs, and Lou Reed's *New York*, which may be one of the few discs that uses alternate CD+G channels for translated lyrics.

I think this application is one of the most correct open source CD+G implementations available. Compare to VLC, which as of writing does not play *Smash Hits* correctly because of an incorrect implementation of `BORDERPRESET`. This seems to be a common error without having seen a disc that scrolls graphics into view.

The code was somewhat updated in 2020 to use SDL2 and published.

# License

[BSD License](./LICENSE.txt)

Copyright 2020 Isaac Brodsky
