
CC = gcc
CFLAGS = -Wall # -Wpedantic -Wextra -Werror
# LDFLAGS = -lpthread
OBJFILES = exfat.o list.o
TARGET = exfat

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) -o $(TARGET) $(OBJFILES) $(LDFLAGS) $(CFLAGS)

clean:
	rm -f $(OBJFILES) $(TARGET) *~
