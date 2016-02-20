.PHONY:clean
.SUFFIXES:.c .o
CC=gcc
EXEC=miniFtpd
FLAG=-Wall -g
SRC=main.c\
	sysutil.c\
	session.c\
	ftpproto.c\
	privateparent.c\
	qstring.c\
	turnable.c\
	parseconf.c\
	privatesock.c
OBJ=$(SRC:.c=.o)
$(EXEC):$(OBJ)
	$(CC) $(FLAG) $^ -o $@ -lcrypt
	@echo "------link ok-------"
.c.o:
	$(CC) $(FLAG) -c $< -o $@
	@echo "------compile ok-----"
clean:
	rm -f $(OBJ)
	rm -f $(EXEC)
