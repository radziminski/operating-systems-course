PROG = tsig
COMP = cc
COMP_FLAGS = -std=c99

compile: tsig.o
	$(COMP) $(COMP_FLAGS) -o $(PROG) tsig.o

tsig.o:
	gcc -c tsig.c

clean: 
	rm tsig.o tsig