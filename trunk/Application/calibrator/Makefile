
CC=g++
DEST=$(CURDIR)/../calibrator/calibrator
CXXFLAGS=-L/usr/X11R6/lib/ -lX11 -I/usr/include/X11 -I/usr/include/cairo -L/usr/lib `pkg-config --libs cairo`
CXXFLAGS+=-D__USE_CAIRO__
# CXXFLAGS+=-D__TEXT_OUT_XORG__

all: $(CURDIR)/../calibrator/calibrator.cpp
	$(CC) $(CXXFLAGS) -o $(DEST) $<

clean:
	rm -f $(DEST)

