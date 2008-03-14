CC=g++
DEST=calibrator
CXXFLAGS=-L/usr/X11R6/lib/ -lX11

all: calibrator.cpp
	$(CC) $(CXXFLAGS) -o $(DEST) $<

clean:
	rm -f $(DEST)
