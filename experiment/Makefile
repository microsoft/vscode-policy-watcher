CFLAGS = -std=c++17 -Wall -framework CoreServices -framework CoreFoundation
CC = clang++

all: test_preferences

test_preferences: test_preferences.cc
	$(CC) $(CFLAGS) -o test_preferences test_preferences.cc

clean:
	rm -f test_preferences
 