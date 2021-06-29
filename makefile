CC = g++
EXE = screenshot

FLAGS = -std=c++17
FLAGS += -Wall
FLAGS += -ID:\SDL64\include\SDL2 -LD:\SDL64\lib
FLAGS += -lmingw32 -lSDL2main -lSDL2

# FLAGS += -Wl,--subsystem,windows

OBJS = main.o

$(EXE): $(OBJS)
	$(CC) $(^:%.o=obj/%.o) $(FLAGS) -o bin/$@ -lgdi32

$(OBJS):
	$(CC) $(FLAGS) -c $(@:%.o=src/%.cc) -o $(@:%.o=obj/%.o)

run:
	./bin/$(EXE).exe

setup:
	mkdir src
	mkdir obj
	mkdir bin

zip:
	zip -r "$(EXE)".zip bin/*

clean:
	rm -f *.exe *.zip obj/*
