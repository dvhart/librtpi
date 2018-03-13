ARCHIVE := rtpi.a
TEST := test
CC := gcc
CFLAGS := -Wall -std=gnu99

.PHONY: default all clean

default: $(TEST)
all: default

OBJECTS = pi_mutex.o pi_cond.o
HEADERS = rtpi.h

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(ARCHIVE): $(OBJECTS) $(HEADERS)
	ar cr $(ARCHIVE) $(OBJECTS)

$(TEST): $(TEST).c $(ARCHIVE) $(HEADERS)
	$(CC) $(TEST).c $(ARCHIVE) -o $(TEST)

clean:
	rm -f *.o
	rm -f $(ARCHIVE)
	rm -f $(TEST)
