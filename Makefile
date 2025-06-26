CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lrt

all: main client

main: main.o emergency.o log.o parse_env.o parse_rescuers.o parse_emergency_type.o
	$(CC) $(CFLAGS) -o main main.o emergency.o log.o parse_env.o parse_rescuers.o parse_emergency_type.o $(LDFLAGS)

client: client.o log.o parse_env.o parse_rescuers.o parse_emergency_type.o
	$(CC) $(CFLAGS) -o client client.o log.o parse_env.o parse_rescuers.o parse_emergency_type.o $(LDFLAGS)

main.o: main.c emergency.h log.h parse_env.h parse_rescuers.h parse_emergency_type.h
	$(CC) $(CFLAGS) -c main.c

client.o: client.c log.h parse_env.h parse_rescuers.h parse_emergency_type.h
	$(CC) $(CFLAGS) -c client.c

emergency.o: emergency.c emergency.h log.h
	$(CC) $(CFLAGS) -c emergency.c

log.o: log.c log.h
	$(CC) $(CFLAGS) -c log.c

parse_env.o: parse_env.c parse_env.h
	$(CC) $(CFLAGS) -c parse_env.c

parse_rescuers.o: parse_rescuers.c parse_rescuers.h
	$(CC) $(CFLAGS) -c parse_rescuers.c

parse_emergency_type.o: parse_emergency_type.c parse_emergency_type.h
	$(CC) $(CFLAGS) -c parse_emergency_type.c

run: main
	./main

clean:
	rm -f *.o main client log.txt client_log.txt
