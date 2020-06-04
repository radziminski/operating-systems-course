NAME = main
OUT = out
COMPILER = cc
CFLAGS = -pthread -std=c99 -Wall

compile: $(NAME).o
	$(COMPILER) $(CFLAGS) -o $(OUT) $(NAME).o

philosophers.o:
	$(COMPILER) -c $(NAME).c

clean:
	rm $(NAME).o $(OUT)
