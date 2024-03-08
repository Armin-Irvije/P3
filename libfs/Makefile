# Variables
lib := libfs.a
CC := gcc
CFLAGS := -Wall -Wextra -Werror
source := disk.c fs.c
obj := $(source:.c=.o)
deps := $(obj:.o=.d)

#M ain
all: $(lib)

#object files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Library creation
$(lib): $(obj)
	ar rcs $@ $^

#dependency tracking
-include $(deps)

clean:
	rm -f *~ $(obj) $(lib) $(deps)
