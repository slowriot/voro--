# Voro++ makefile
#
# Author : Chris H. Rycroft (LBL / UC Berkeley)
# Email  : chr@alum.mit.edu
# Date   : July 1st 2008

# Load the common configuration file
include ../../config.mk

# The relative of path of the main library source files
INCLUDE=-I../../src

# List of the common source files
SOURCE=../../src/container.cc ../../src/container.hh ../../src/config.hh \
       ../../src/cell.hh ../../src/cell.cc ../../src/wall.cc ../../src/wall.hh \
       ../../src/worklist.cc ../../src/worklist.hh ../../src/voro++.cc \
       ../../src/voro++.hh

# List of executables
EXECUTABLES=cut_region box_cut superellipsoid

# Makefile rules
all: cut_region box_cut superellipsoid

superellipsoid: $(SOURCE) superellipsoid.cc
	$(CC) $(CFLAGS) $(INCLUDE) -o superellipsoid superellipsoid.cc

cut_region: $(SOURCE) cut_region.cc
	$(CC) $(CFLAGS) $(INCLUDE) -o cut_region cut_region.cc

box_cut: $(SOURCE) box_cut.cc
	$(CC) $(CFLAGS) $(INCLUDE) -o box_cut box_cut.cc

clean:
	rm -f $(EXECUTABLES)
