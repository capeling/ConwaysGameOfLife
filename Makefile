C_FLAGS = -Wall -Wextra -std=c99 -pedantic `pkg-config --cflags sdl3` $(COMMON_CFLAGS)
LIBS = -lm `pkg-config --libs sdl3` $(COMMON_LIBS)
SRC = main.c font.c
OBJ = ${SRC:.c=.o}

all: ConwaysGameOfLife

.c.o:
	${CC} -c ${C_FLAGS} $<

ConwaysGameOfLife: ${OBJ}
	${CC} -o $@ ${OBJ} ${LIBS}

clean:
	rm -f ${OBJ} ConwaysGameOfLife

run: ConwaysGameOfLife
	./ConwaysGameOfLife
