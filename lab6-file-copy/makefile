NAME = copy
COMP = cc
FLAGS = -Wall -std=c99

compile: copy.o
	$(COMP) $(FLAGS) -o $(NAME) $(NAME).o

copy.o:
	$(COMP) -c $(NAME).c

clean:
	rm $(NAME).o $(NAME)
