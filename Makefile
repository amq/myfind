CC         = clang
EXECUTABLE = myfind
CCFLAGS    = -c -g -std=gnu11 -fno-common -O3
CCWARNS    = -Wall -Wextra -Wstrict-prototypes -pedantic
LDFLAGS    =

.PHONY: clean

all: $(EXECUTABLE)

$(EXECUTABLE): main.o
  $(CC) $(LDFLAGS) main.o -o $(EXECUTABLE)

main.o:
  $(CC) $(CCFLAGS) $(CCWARNS) main.c

clean:
  rm -f *.o
  rm -f $(EXECUTABLE)
