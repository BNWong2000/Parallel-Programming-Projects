CC = mpicxx 
SRCS = ./src/*.cpp
INC = ./src/
OPTS = -std=c++17 -g -Wall -o3 -Werror  -lglfw3 -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lGLEW

EXEC = nbody

all: clean compile

compile:
	$(CC) $(SRCS) $(OPTS) -I$(INC) -o $(EXEC)

clean:
	rm -f $(EXEC)
