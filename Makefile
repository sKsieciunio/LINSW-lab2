CC     ?= gcc
CFLAGS += -Wall -Wextra
LDLIBS  = -lperiphery -lpthread

TARGET = sequencer
SRC    = sequencer.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(TARGET)
