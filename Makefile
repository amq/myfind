TARGET   = myfind
CC       = clang
CFLAGS   = -c -g -coverage -std=gnu11 -fno-common -O0
CWARNS   = -Wall -Wextra -Wstrict-prototypes -pedantic
LDFLAGS  = -coverage

.PHONY: clean format

all: $(TARGET)

$(TARGET): main.o
	$(CC) $(LDFLAGS) main.o -o $(TARGET)

main.o:
	$(CC) $(CFLAGS) $(CWARNS) main.c

clean:
	rm -f *.o
	rm -f $(TARGET)

format:
    clang-format -style=llvm -i *.c *.h
