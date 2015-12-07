CFLAGS+=-D_FILE_OFFSET_BITS=64 -fPIC
CXXFLAGS+=$(CFLAGS) -std=gnu++14
LINKLIB=g++ -o $@ $^ -shared -lm -lprotobuf -levent -luuid -lleveldb
LINK=g++ -o $@ $(filter %.o, $^) $(LDFLAGS) -Wl,-rpath=. -L. -ldpfs

ifeq ($(dbg), 1)
CFLAGS+=-DDEBUG -g -O0
LDFLAGS+=-g
else
CFLAGS+=-O2
endif

targets=dpfs dpfsMonitor dpfsOSD
library=libdpfs.so
all:$(targets) $(library)

protobuf_srcs=$(patsubst %.proto,%.pb.cc,$(wildcard *.proto))
protobuf_headers=$(patsubst %.proto,%.pb.h,$(wildcard *.proto))

libSrcs=$(filter-out $(patsubst %,%.cc,$(targets)), $(wildcard *.cc) $(wildcard *.c))
libObjs=$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(libSrcs))) $(patsubst %.cc,%.o,$(protobuf_srcs))

$(libObjs) dpfs.o dpfsMonitor.o: $(protobuf_headers)

$(library): $(libObjs)
	$(LINKLIB)

dpfs: dpfs.o $(library)
	$(LINK) -lfuse -lprotobuf -lpthread

dpfsMonitor: dpfsMonitor.o $(library)
	$(LINK) -lprotobuf

dpfsOSD: dpfsOSD.o $(library)
	$(LINK) -lprotobuf

clean:
	rm -f $(targets) *.pb.h *.pb.cc *.o *.so

%.pb.h %.pb.cc: %.proto
	protoc --cpp_out=. $^
