CC = gcc
CFLAGS =  -g3 -Wall -Wextra -Werror -pedantic -Iinclude
PROGRAM = main
OBJS = main.o my_allocator.o

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(PROGRAM)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(PROGRAM) $(OBJS)