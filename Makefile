CC = gcc
CFLAGS = -I. -Wall -pedantic -std=c11
NAME = main
LIBS = -lpthread
OBJS = $(NAME).o parse_env.o parse_rescuers.o parse_emergency_type.o 

.PHONY: default clean run

default: $(NAME)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

run: $(NAME)
	./$(NAME)

clean:
	rm -f $(NAME) $(OBJS)
