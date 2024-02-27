# library
lib := libfs.a

CC := gcc
CFLAGS := -Wall -Wextra -Werror

# Source files

source := disk.c fs.c
obj := $(source:.c=.o)

all: $(lib)
#puts the object files into the library
$(lib): $(obj)
	ar rcs $@ $^ 

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
#clean all object files
clean:
	rm -f *~ $(obj) $(lib)
