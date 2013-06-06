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
ported it to pure C. I've moved it to a separate repository
(github.com/bluepeppers/libsg), and to build on the libsg branch of sgreader,
you need to install it first. This makes the entire process:

    git clone git://github.com/bluepeppers/libsg.git
    cd libsg/c
    make
    sudo make install

    # Now go to sgreader dir
    git checkout libsg
    qmake
    make
    ./sgreader
