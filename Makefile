#!/usr/bin/make -f

CC = clang
MKDIR = mkdir -p
CFLAGS = \
		-std=c99 \
		-m64 \
		-O4 \
		-g \
		-g3 \
		-I$(INCLUDESDIR) \
		-Wall \
		-Wextra \
		-Wformat \
		-Wno-unused-parameter \
		-Werror=format-security
LDFLAGS = -lm
LIBDIR = ./lib
SRCDIR = ./src
BINDIR = ./bin
INCLUDESDIR = ./include
APPSERNAME = jacobi-serial
APPPARNAME = jacobi-parallel
APPUTILS = jacobiutils

.PHONY: all
all: clean $(APPUTILS) makebindir $(APPSERNAME) $(APPPARNAME) doc

$(APPUTILS): \
		$(LIBDIR)/matrixutils.c \
		$(INCLUDESDIR)/matrixutils.h \
		$(LIBDIR)/jacobi.c \
		$(INCLUDESDIR)/jacobi.h \
		$(LIBDIR)/mpiutils.c \
		$(INCLUDESDIR)/mpiutils.h
	-rm -f $(LIBDIR)/lib$(APPUTILS).a
	$(CC) $(CFLAGS) -c $(LIBDIR)/*.c 
	mv *.o $(LIBDIR)/
	ar -cvq $(LIBDIR)/lib$(APPUTILS).a $(LIBDIR)/*.o
	rm $(LIBDIR)/*.o

.PHONY: makebindir
makebindir: $(BINDIR)/

$(BINDIR)/:
	$(MKDIR) $@

.PHONY: $(APPSERNAME)
$(APPSERNAME): $(LIBDIR)/lib$(APPUTILS).a $(SRCDIR)/$(APPSERNAME).c
	$(CC) $(CFLAGS) $(SRCDIR)/$(APPSERNAME).c $(LDFLAGS) -L$(LIBDIR) \
		-l$(APPUTILS) -o $(BINDIR)/$(APPSERNAME)

.PHONY: $(APPPARNAME)
$(APPPARNAME): $(LIBDIR)/lib$(APPUTILS).a $(SRCDIR)/$(APPPARNAME).c
	mpicc $(CFLAGS) -no-pie $(SRCDIR)/$(APPPARNAME).c $(LDFLAGS) -L$(LIBDIR) \
		-l$(APPUTILS) -o $(BINDIR)/$(APPPARNAME)

.PHONY: doc
doc: Doxyfile
	-doxygen Doxyfile
	-moxygen -g -a -o ./doc/jacobi-mpi-%s.md ./doc/xml/
	#-rm -r doc/xml/

clean:
	-rm $(BINDIR)/$(APPSERNAME)
	-rm $(BINDIR)/$(APPPARNAME)
	-rm -r ./doc/xml/
	-rm -r ./doc/*.md
