TARGET   = myfind
CC       = clang
CFLAGS   = -c -g -std=gnu11 -fno-common -O3
CWARNS   = -Wall -Wextra -Wstrict-prototypes -pedantic
LDFLAGS  =

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
