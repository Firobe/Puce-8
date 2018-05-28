COMPILER = g++
ADD_CFLAGS = -Wall -std=c++11 -O3
LIBS = glu glfw3 openal
NO_PKG_LIBS =
CXXFLAGS = `pkg-config --static --cflags $(LIBS)` $(ADD_CFLAGS)
LINKS = `pkg-config --static --libs $(LIBS)` $(NO_PKG_LIBS)
SOURCES = $(wildcard src/*.cpp)
EXEC_NAME = puce8
OBJECTS = $(SOURCES:.cpp=.o)
.PRECIOUS : %.o
.PHONY : clean
.PHONY : run

all: $(OBJECTS)
	$(COMPILER) -L/usr/lib -o $(EXEC_NAME) $(OBJECTS) $(LINKS)

%.o : %.cpp %.h
	$(COMPILER) -c $(CXXFLAGS) $< -o $@

run: all
	./$(EXEC_NAME)

clean:
	rm -f $(OBJECTS) $(EXEC_NAME)
