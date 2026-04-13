CC = gcc
CFLAGS = -Wall -Wextra -g
LDLIBS = -lm

SRCS = $(wildcard *.c)
NAMES = $(SRCS:%.c=%)
BINS = $(NAMES:%=%.out)

all: $(BINS)

%.out: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

.PHONY: $(NAMES)
$(NAMES): %: %.out

clean:
	rm -f $(BINS)

clean-%:
	rm -f $*.out
