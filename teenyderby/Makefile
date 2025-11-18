
CFLAGS += -I.. -O3
WFLAGS = -Wall -Wextra -Wpedantic -Wno-unused-parameter
CPPFLAGS += $(CFLAGS) --std=c++17
EXEC = teenyderby

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LDFLAGS += -framework OpenGL -framework Cocoa
else ifeq ($(UNAME_S),Linux)
	LDFLAGS += -s -lGLU -lGL -lX11
endif

all:
	gcc -c ../tigr.c -o tigr.o $(CFLAGS)
	gcc -c ../teenyat.c -o teenyat.o $(CFLAGS) $(WFLAGS)
	g++ -c main.cpp -o main.o $(CPPFLAGS) $(WFLAGS)
	g++ tigr.o teenyat.o main.o -o $(EXEC) $(LDFLAGS)

clean:
	rm -f $(EXEC) *.o
