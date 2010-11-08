CFLAGS = -O3 -march=native -fomit-frame-pointer -flto -fwhole-program

all: minifind

C_FILES = minifind.c filefind.c

minifind: $(C_FILES)
	gcc `pkg-config --cflags --libs glib-2.0` $(CFLAGS) -o $@ $(C_FILES)

clean:
	rm -f minifind *.o
