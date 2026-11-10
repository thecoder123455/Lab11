
CC = gcc
CFLAGS = -Wall -g -MMD -MP 
LDFLAGS = -ljpeg -lm

EXECUTABLES = mandel mandelmovie
OBJECTS = mandel.o mandel_core.o mandelmovie.o jpegrw.o

all: $(EXECUTABLES)

# Include dependency information
-include $(OBJECTS:.o=.d)

# Original Mandelbrot program
mandel: mandel.o jpegrw.o
	$(CC) $^ $(LDFLAGS) -o $@

# Mandelbrot movie program
mandelmovie: mandelmovie.o mandel_core.o jpegrw.o
	$(CC) $^ $(LDFLAGS) -o $@

# Compile Mandelbrot functions without main()
mandel_core.o: mandel.c
	$(CC) $(CFLAGS) -DMANDEL_LIBRARY -c $< -o $@

# Compile remaining source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean generated files
clean:
	rm -f $(OBJECTS) $(EXECUTABLES) *.d

.PHONY: all clean
