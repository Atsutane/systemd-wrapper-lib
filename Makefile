CXX=c++
CXXFLAGS= -I. -O2 -g -Wall -Wextra -Werror -fstack-protector -std=c++11
CXXFLAGS+= -DSDW_DLSYM
LDFLAGS= -L.

INDENT_ARGS = -linux -i4 -nut -nbfda -il0 -cli4 -cs -brf

all: sdwc aux_scope_daemon

sdwc: sdwc.o libsdw.a
	$(CXX) -o $@ sdwc.o $(LDFLAGS) -lsystemd -lsdw -ldl

sdwc.o: sdwc.cpp sdw.h
	$(CXX) $(CXXFLAGS) -c -o $@ sdwc.cpp

sdw.o: sdw.cpp sdw.h
	$(CXX) $(CXXFLAGS) -c -o $@ sdw.cpp

libsdw.a: sdw.o
	$(AR) -r $@ sdw.o

aux_scope_daemon: aux_scope_daemon.o libsdw.a
	$(CXX) -o $@ aux_scope_daemon.o $(LDFLAGS) -lsystemd -lsdw -ldl -luuid

aux_scope_daemon.o: aux_scope_daemon.cpp sdw.h
	$(CXX) $(CXXFLAGS) -c -o $@ aux_scope_daemon.cpp

clean:
	@rm -f sdwc sdwc.o sdw.o libsdw.a aux_scope_daemon aux_scope_daemon.o

.PHONY: indent
indent:
	@indent $(INDENT_ARGS) --preserve-mtime sdw.cpp
	@indent $(INDENT_ARGS) --preserve-mtime sdwc.cpp
	@indent $(INDENT_ARGS) --preserve-mtime aux_scope_daemon.cpp
