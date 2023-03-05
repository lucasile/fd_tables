CC=gcc
LIBS=-lm
ARGS=-Wall

fd_tables: fd_tables.c structprocess.h
	$(CC) $< $(ARGS) $(LIBS) -o $@ 
clean:
	$(RM) $(TARGET)

