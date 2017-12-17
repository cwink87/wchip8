CPP := clang++

CUSTOMFLAGS := $(CUSTOMFLAGS) -Weverything -Wno-c++98-compat -Wno-used-but-marked-unused -pedantic-errors -std=c++14 -g -O0

SDLFLAGS := $(SDLFLAGS) $(shell sdl2-config --cflags)
SDLLIBS := $(SDLLIBS) $(shell sdl2-config --libs)

CPPFLAGS := $(CPPFLAGS) $(CUSTOMFLAGS) $(SDLFLAGS)
CPPLIBS := $(CPPLIBS) $(SDLLIBS)

main.x: main.o
	$(CPP) -o $@ $^ $(CPPLIBS)

main.o: main.cpp
	$(CPP) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -rf *.o
	rm -rf *.x

run:
	./main.x

