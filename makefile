CXXFLAGS=-D_FILE_OFFSET_BITS=64 -std=gnu++14
LDFLAGS=-lfuse -lm
all: dpfs

dpfs: $(wildcard *.cc)

clean:
	rm -f dpfs
