# CD+G Deck

CD+G Deck is an application for playing Compact Disc Graphics. These were graphics tracks embedded in the "subcode" fields of audio CDs. A limited slelection of CDs from the later 80's have elaborate but low bitrate graphics tracks embedded in them. You 
can learn more about CD+G at the [CD+G Museum](http://www.cdplusg.com/cdplusg/Main.html). This happened in the same era as the Philips CD-i, and CD Video (not to be confused with Video CD). After this initial commercial push, CD+G retreated from public 
view with the exception of karaoke CDs, which have made rigorous use of the format.

There was also a CD+EG (Compact Disc Extended Graphics) format, but I am not aware of any discs using this standard ever being made. The Sega Saturn may have a reference decoder for the CD+EG format.

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

```
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake ..
make
```

# History

This code was originally written around 2012-2013. It was mainly tested against *Information Society* (the disc I originally learned about CD+G from), and then later against Jimi Hendrix Experience' *Smash Hits* (probably one of the most technically and 
artistically developed CD+G discs.) Lour Reed's *New York* may be one of the few discs that support alternate CD+G channels, for translated lyrics. I think this application is one of the most correct open source CD+G implementations available. 
Compare to VLC, which as of writing does not play *Smash Hits* correctly because of an incorrect implementation of `BORDERPRESET`.

The code was somewhat updated in 2020 to use SDL2 and published.

# Legal

License TBD
Copyright 2020 Isaac Brodsky
