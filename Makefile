CXX=c++
CXXFLAGS= -I. -O2 -g -Wall -Wextra -Werror -fstack-protector -std=c++11
CXXFLAGS+= -DSDW_DLSYM
LDFLAGS= -L.

INDENT_ARGS = -linux -i4 -nut -nbfda -il0 -cli4 -cs -brf

all: sdwc

sdwc: sdwc.o libsdw.a
	$(CXX) -o $@ sdwc.o $(LDFLAGS) -lsystemd -lsdw -ldl

sdwc.o: sdwc.cpp sdw.h
	$(CXX) $(CXXFLAGS) -c -o $@ sdwc.cpp

sdw.o: sdw.cpp sdw.h
	$(CXX) $(CXXFLAGS) -c -o $@ sdw.cpp

libsdw.a: sdw.o
	$(AR) -r $@ sdw.o

clean:
	@rm -f sdwc sdwc.o sdw.o libsdw.a

.PHONY: indent
indent:
	@indent $(INDENT_ARGS) --preserve-mtime sdw.cpp
	@indent $(INDENT_ARGS) --preserve-mtime sdwc.cpp
