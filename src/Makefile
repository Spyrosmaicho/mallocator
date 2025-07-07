CC = gcc
CFLAGS =  -g3 -Wall -Wextra -Werror -pedantic
PROGRAM = main
OBJS = main.o my_malloc.o

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(PROGRAM)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(PROGRAM) $(OBJS)