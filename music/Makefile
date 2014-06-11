
PKGS = sdl2

CC=clang++
CFLAGS = --std=c++11 -msse -I.. -m64 -O3 -Wall $(shell pkg-config $(PKGS) --cflags)
LINK = $(shell pkg-config $(PKGS) --libs) -lm

all: adsr smplr smplbx

adsr: adsr.cc
	$(CC) $(CFLAGS) $(LINK) adsr.cc -o adsr

smplr: smplr.cc
	$(CC) $(CFLAGS) $(LINK) smplr.cc -o smplr

smplbx: smplbx.cc
	$(CC) $(CFLAGS) $(LINK) smplbx.cc -o smplbx

clean:
	rm -rf adsr smplr smplr2 smplbx

