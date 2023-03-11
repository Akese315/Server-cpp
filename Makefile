CC = g++
CFLAGS = -Wall -g

Object = main.o Server.o PlayerManager.o Player.o Client.o EventData.o Console.o Log.o Protocol.o EventManager.o

server.out : $(Object)
	$(CC) $(CFLAGS) $(Object) -o server.out

Server.o : Server.cpp 
	$(CC) $(CFLAGS) -c $< -o $@

PlayerManager.o : PlayerManager.cpp
	$(CC) $(CFLAGS) -c $< -o $@

Player.o : Player.cpp 
	$(CC) $(CFLAGS) -c $< -o $@

Client.o : Client.cpp 
	$(CC) $(CFLAGS) -c $< -o $@

EventData.o : EventData.cpp 
	$(CC) $(CFLAGS) -c $< -o $@

Console.o : Console.cpp
	$(CC) $(CFLAGS) -c $< -o $@

Log.o : Log.cpp
	$(CC) $(CFLAGS) -c $< -o $@

Protocol.o : Protocol.cpp
	$(CC) $(CFLAGS) -c $< -o $@

EventManager.o : EventManager.cpp
	$(CC) $(CFLAGS) -c $< -o $@
	
main.o : main.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean : 
	rm *.o
	rm *.out
