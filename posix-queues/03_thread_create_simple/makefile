CC=gcc 
CFLAGS=-std=gnu99 -Wall -fsanitize=thread,undefined
L_FLAGS=-fsanitize=thread,undefined
LDLIBS=-lpthread -lm -lrt
TARGET=main 
FILES=main.o

${TARGET}: ${FILES} 
	${CC} -o ${TARGET} ${FILES} ${L_FLAGS} ${LDLIBS}
 
main.o: main.c
	${CC} -o main.o -c main.c ${C_FLAGS} 
 
.PHONY: clean 
 
clean: 
	-rm -f ${FILES} ${TARGET}

.PHONY: rebuild

rebuild:
	make clean
	make
