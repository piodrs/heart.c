GCC = gcc 
CFLAGS = -std=c11 -O2 -Wall -Wextra
LDLIBS = -lm

heart: heart.c
	$(GCC) $(CFLAGS) $< -o $@ $(LDLIBS)
