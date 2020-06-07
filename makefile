#------------------------------------------------------------------------------
# Copyright (c) 2020 Richard Michael Thomas <rmthomas@sciolus.org>
#
#  makefile for project COV-SWN
#
#------------------------------------------------------------------------------
SHELL = /bin/bash
##CC = gcc -ansi -std=c99 -pedantic -Wall -Wno-misleading-indentation
##CC = gcc -ansi -std=gnu99 -pedantic -Wall -Wno-misleading-indentation

#CC = clang -pedantic -Wall
CC = gcc -pedantic -Wall

.PHONY:		ALL
.PHONY:		GRIND
.PHONY:		DOXYGEN
.PHONY:		clean
ALL:		cov gracov demo
GRIND:		cov.c swn.o demo.c clean
		$(CC) -g -o cov -lm swn.o cov.c
		$(CC) -g -o demo demo.c
cov:		cov.c swn.o makefile
		$(CC) -o cov -lm swn.o cov.c
swn.o:		swn.c makefile
		$(CC) -c swn.c
gracov:		gracov.c makefile
		$(CC) -o gracov -lm gracov.c
demo:		demo.c makefile
		$(CC) -o demo -lm demo.c
DOXYGEN:
		@if [ \( -n "`which doxygen`" \) -a \
                      \( -e doxygen.config \) ]; then \
		OD="`cat doxygen.config | grep "OUTPUT_DIRECTORY" | \
			head -n 1 | sed "s,^.*=,," `"; \
                1>dox.log 2>dox.err doxygen doxygen.config; \
                if [ -s dox.err ]; then ls -l dox.*; fi; \
		echo "Output is in directory $${OD}"; fi
clean:	
		@rm -f swn.o cov gracov demo
