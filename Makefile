EXE = sigmund
CXX = g++
SOURCES := 	$(wildcard src/*.cpp)
HEADERS := 	$(wildcard src/*.h)
OBJECTS := $(SOURCES:.cpp=.o)

INCLUDES +=	-Isrc/ -Iinclude/
CFLAGS += -Wall -O2 -std=c++11 -fpermissive 
LDFLAGS += -lasound -lfftw3f -lm -pthread -llo

all: $(EXE)

%.o: %.cpp
	@echo $@
	$(CXX) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations

$(EXE): $(OBJECTS) $(HEADERS)
	$(CXX) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -rdynamic -o $@

clean:
	@rm -rvf $(EXE) src/*.o

install:
	@cp $(EXE) /usr/local/bin
