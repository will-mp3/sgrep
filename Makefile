CFLAGS= -Wall -Wextra -Werror -ggdb

prog = sgrep
objects = sgrep.o mu.o
headers = mu.h list.h

$(prog): $(objects)
	$(CC) -o $@ $^

$(objects) : %.o : %.c $(headers)
	$(CC) -o $@ -c $(CFLAGS) $<

clean:
	rm -f $(prog) $(objects)
