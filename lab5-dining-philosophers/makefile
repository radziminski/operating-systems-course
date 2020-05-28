NAME = main
OUT = out
COMPILER = cc
CFLAGS = -std=c99

compile: $(NAME).o
	$(COMPILER) $(FLAGS) -o $(OUT) $(NAME).o

philosophers.o:
	$(COMPILER) -c $(NAME).c

clean:
	rm $(NAME).o $(OUT)
