sysUpdater2 by paulguy

This is a rewrite of sysUpdater based on SafeSysUpdater by cpasjuste.  I was
very careful to consider all possible dangers to see if I could improve
reliability, but it's not possible to consider everything in a black box like
the 3DS.

                              !!!!! WARNING !!!!!
This is currently untested!  Everything else seems to work fine (still may be
bugs there, too!) but the code to actually delete or install anything has not
been tested at all!  Even if this was tested, USE AT YOUR OWN RISK!  It could
cost you!
                              !!!!! WARNING !!!!!

Build:
cmake -DCMAKE_TOOLCHAIN_FILE=DevkitArm3DS.cmake .
make

Usage:
Either copy the .3dsx and .smdh to your SD card in a directory under /3ds/ or
install the .cia with your CIA installer of choice.

Copy your CIAs to /updates/ on the SD card.

Run it.

If the exploit doesn't seem to work, try running this or some other application
then quitting back to the menu then running this application.  This seems to be
more reliable, but it can still fail sometimes.
