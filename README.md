SGReader
========

History
-------

This program was original written by Pecunia (http://pecunia.nerdcamp.net/).
I'm placing it here so that should the various places that currently host a copy
of the source code disappear, the world will still be able to extract SG files.

Use
---

SG files refer to the data files of the City Building games, i.e. Zeus,
Caesar 3, and such like. SGreader will extract sg3 and sg2 files.

Build
-----

    qmake
    make
    ./sgreader
    # or
    sudo make install

If you want to build the original SGReader without any modifications, checkout
the git tag "pecunia".

LibSG
=====

I've separated out the core SG/555 reading, removed all QT dependencies and
ported it to pure C. If you want to use it, just copy it from the libsg
directory. If anyone actually knows how to write a decent makefile to support
installing/building it as a shared/static library, please contribute.
