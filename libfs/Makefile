CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Object files
OBJS = tester.o fs.o disk.o

# Executable name
EXEC = tester

# Default rule to compile the executable
all: $(EXEC)

# Rule to compile tester.o
tester.o: tester.c fs.h disk.h
	$(CC) $(CFLAGS) -c tester.c

# Rule to compile fs.o
fs.o: fs.c fs.h disk.h
	$(CC) $(CFLAGS) -c fs.c

# Rule to compile disk.o
disk.o: disk.c disk.h
	$(CC) $(CFLAGS) -c disk.c

# Rule to link the object files and create the executable
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

# Clean rule to remove object files and the executable
clean:
	rm -f $(OBJS) $(EXEC)
