DIROBJ := obj/
DIREXE := exec/
DIRHEA := include/
DIRSRC := src/

CC = mpicc
RUN = mpirun
CFLAGS  = -c -I$(DIRHEA) -g3
RFLAGS =
LDLIBS = -lSDL2


all: clean dirs main

dirs:
	mkdir -p $(DIROBJ) $(DIREXE)

$(DIROBJ)%.o: $(DIRSRC)%.c
	$(CC) $(CFLAGS) $^ -o $@

main: $(DIROBJ)main.o $(DIROBJ)game.o $(DIROBJ)utils.o 
	$(CC) -o $(DIREXE)$@ $^ $(LDLIBS)

game: $(DIROBJ)game.o 
	$(CC) -o $(DIREXE)$@ $^ $(LDLIBS)

run:
	mpirun -n 16 --oversubscribe ./exec/main

clean:
	rm -rf *~ core $(DIROBJ) $(DIREXE) $(DIRHEA)*~ $(DIRSRC)*~
