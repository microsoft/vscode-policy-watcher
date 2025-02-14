CFLAGS = -Wall -framework CoreServices
CC = clang

all: test_preferences

test_preferences: test_preferences.c
	$(CC) $(CFLAGS) -o test_preferences test_preferences.c

clean:
	rm -f test_preferences
 