INC=-Iinclude
CFLAGS=$(INC)

PARSERDIR=input_parse

SRCS=utils.c mshell.c builtins.c pipe.c debug.c bgproc_queue.c process_manager.c signal_utils.c reader.c message.c config.c
OBJS:=$(SRCS:.c=.o)

all: mshell 

mshell: $(OBJS) siparse.a
	cc $(CFLAGS) $(OBJS) siparse.a -o $@ 

%.o: %.c
	cc $(CFLAGS) -c $<

siparse.a:
	$(MAKE) -C $(PARSERDIR)

clean:
	rm -f mshell *.o
