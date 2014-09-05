CXX     :=g++
TARGET  :=openbeacon-rx
SOURCES :=src/bmMapHandleToItem.cpp src/main.cpp src/crypto.cpp
FILTERSS:=filter-singularsighting
LIBS    :=-lm -pthread

# determine program version
PROGRAM_VERSION:=$(shell git describe --tags --abbrev=4 --dirty 2>/dev/null | sed s/^v//)
ifeq ("$(PROGRAM_VERSION)","")
    PROGRAM_VERSION:='unknown'
endif

# determine if we have custom encryption keys
ifeq ("$(wildcard src/custom-encryption-keys.h)","")
    ENCRYPTION_KEYS:=
else
    ENCRYPTION_KEYS:=-DCUSTOM_ENCRYPTION_KEYS
endif

DEBUG   :=-O3
CXXOPT  :=$(DEBUG)
INCLUDE :=-I../../firmware/nRF51/tag-proximity/inc/
CXXFLAGS:=-Werror -Wall -D_REENTRANT -DPROGRAM_VERSION=\"$(PROGRAM_VERSION)\" -DPROGRAM_NAME=\"$(TARGET)\" $(ENCRYPTION_KEYS) $(INCLUDE)
LDFLAGS :=$(LIBS)
OBJS    :=$(SOURCES:%.cpp=%.o)
LOGJSON :=$(LOGFILE:%.bin=%.json.bz2)
PREFIX  :=/usr/local

all: $(TARGET) $(FILTERSS)

demo: all
	./$(TARGET) | ./$(FILTERSS) ~/public_html/test.json

.PHONY: all version run install indent debug dependencies-fedora cleanall clean

version:
	@echo "$(TARGET) version $(PROGRAM_VERSION)"

run: $(TARGET)
	./$(TARGET)

install: $(TARGET) $(FILTER)
	install $(TARGET) $(FILTER) $(PREFIX)/bin/

$(FILTERSS): src/$(FILTERSS).cpp
	$(CXX) $(CXXFLAGS) $(CXXOPT) $(LDOPT) $^ -lz -o $@

indent: $(SOURCES)
	find src -iname '*\.[cph]*' -exec indent -c81 -i4 -cli4 -bli0 -ts 4 \{\} \;
	rm -f src/*~

$(TARGET): .depend $(OBJS)
	$(CXX) $(LDOPT) $(OBJS) $(LDFLAGS) -o $@

.depend: $(SOURCES) $(FILTERS)
	$(CXX) $(CXXFLAGS) $(CXXOPT) -MM $^ > $@

.cpp.o:
	$(CXX) $(CXXFLAGS) $(CXXOPT) -c $< -o $@

dependencies-fedora:
	sudo yum install gcc-c++ zlib zlib-devel

cleanall: clean
	rm -f .depend

clean:
	rm -f $(TARGET) $(OBJS) $(FILTERSS) *~

include .depend
