CC = gcc 
HTTPOPTS = -c -g --pedantic -Wall
OOPTS = $(HTTPOPTS) --ansi

# on solaris we definitely need this. this is insane, i know, i just
# couldn't find a better solution.
SOLARIS = `if [ \`uname -s \` = "SunOS" ]; then echo "-lsocket -lnsl"; fi`

PROG = twitterm
OBJS = base64.o http.o json_reader.o json_writer.o ui.o main.o

.SUFFIXES = .c

bin:$(OBJS)
	$(CC) $(OBJS) -o $(PROG) $(SOLARIS)

http.o:
	# turning off --ansi, since the APIs used aren't ANSI but POSIX
	$(CC) $(HTTPOPTS) http.c
	
.c.o:
	$(CC) $(OOPTS) $*.c
	
clean:
	rm -f $(OBJS) $(PROG)
