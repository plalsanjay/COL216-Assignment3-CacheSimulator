CC = g++
CFLAGS = -std=c++14 -Wall -O2
TARGET = L1simulate

SOURCES = main.cpp cache_simulator.cpp cache.cpp cache_set.cpp bus.cpp core.cpp
HEADERS = cache_simulator.h

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

.PHONY: all clean
