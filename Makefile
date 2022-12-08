CFLAGS = -O
CC = g++ -std=c++11 

SRC = node.cpp utility.cpp server.cpp client.cpp
OBJ = $(SRC:.cpp = .o)

node: $(SRC)
	$(CC) $(CFLAGS) -o node $(OBJ) -lpthread

clean:
	rm -f core *.o
