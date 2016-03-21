CC = gcc
EXECS = 33sh 
#33noprompt
CFLAGS = -g3 -Wall -Wextra -Wconversion -Wcast-qual -Wcast-align -g
CFLAGS += -Winline -Wfloat-equal -Wnested-externs
CFLAGS += -pedantic -std=gnu99 -Werror

PROMPT = -DPROMPT
all: $(EXECS)

33sh: sh.c
	$(CC) $(CFLAGS) -DPROMPT sh.c -o sh
	
33noprompt:
	$(CC) $(CFLAGS) sh.c -o sh
clean:
	rm -f $(EXECS)

