CC = winegcc
CFLAGS ?= -O2
LDFLAGS ?= -lX11

default: nightcap.exe

%.exe: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)
