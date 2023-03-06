CC=gcc
LIBS=-lm
ARGS=-Wall
RM=rm
OUTPUT_FILES=compositeTable.txt compositeTable.bin

fd_tables: fd_tables.c structprocess.h
	$(CC) $< $(ARGS) $(LIBS) -o $@ 

.PHONY: clean_output
clean-output:
	$(RM) $(OUTPUT_FILES)

.PHONY: clean
clean:
	$(RM) fd_tables

