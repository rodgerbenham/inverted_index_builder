# Blocked Sort-Based Indexer
C-based BSBI Inverted Index Builder for Stanford CS276 corpus.

---

## Preliminaries:
Make a folder called `./docs` and copy the folders 0-9 from the [Stanford A1 Corpus](http://web.stanford.edu/class/cs276/pa/pa1-data.zip) into the directory.

Make a folder called `./intermediates`. The in-memory sorted / collected postings lists are stored in this folder.

Ensure you have the GNOME GLib Library installed with the headers.

## How to build:

This project uses the [tup](http://gittup.org/tup/) build system.

To build use `tup init` to initialise the directory for use with tup, and then `tup upd` to compile the executable.

## Usage

Run by invoking `./main` in your shell.

The files output are `./doc.dict`, `./term.dict` and importantly `./posting.dict` - the index we want.

## Experimental Results

It took 12 minutes and 15 seconds to complete indexing on the Stanford A1 corpus. The program consumed 388MB of RAM, in a virtual machine on an Intel i5-2400 CPU @ 3.10GHz with DDR3 memory, and a Seagate Barracuda 7200RPM SATA HDD with 64MB of cache.

RAM usage grows with respect to new terms, because the BSBI algorithm needs a data structure for mapping terms to termIDs.

## Improvements

The code could use some refactoring and breaking out into seperate files for comparators, file manipulation functions etc.

There is no support for compression and would very much benefit from gamma encoding.

There is presently no support for stop words.
