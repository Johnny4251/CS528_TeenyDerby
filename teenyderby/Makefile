CFLAGS  += -I.. -O3
WFLAGS   = -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-but-set-variable
CPPFLAGS += $(CFLAGS) --std=c++17
EXEC     = teenyderby

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LDFLAGS += -framework OpenGL -framework Cocoa
else ifeq ($(UNAME_S),Linux)
	LDFLAGS += -s -lGLU -lGL -lX11
endif

all:
	gcc -c ../tigr.c    -o tigr.o    $(CFLAGS)
	gcc -c ../teenyat.c -o teenyat.o $(CFLAGS) $(WFLAGS)
	g++ -c utils.cpp    -o utils.o   $(CPPFLAGS) $(WFLAGS)
	g++ -c bus.cpp      -o bus.o     $(CPPFLAGS) $(WFLAGS)
	g++ -c menu.cpp     -o menu.o    $(CPPFLAGS) $(WFLAGS)
	g++ -c game.cpp     -o game.o    $(CPPFLAGS) $(WFLAGS)
	g++ -c main.cpp     -o main.o    $(CPPFLAGS) $(WFLAGS)
	g++ tigr.o teenyat.o main.o utils.o bus.o menu.o game.o -o $(EXEC) $(LDFLAGS)

clean:
	rm -f $(EXEC) *.o
