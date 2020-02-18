#!/usr/bin/make -f

CC = clang
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
INCLUDESDIR = ./include
APPSERNAME = jacobi-serial
APPPARNAME = jacobi-parallel
APPUTILS = jacobiutils

.PHONY: all
all: clean $(APPUTILS) $(APPSERNAME) $(APPPARNAME) doc

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

.PHONY: $(APPSERNAME)
$(APPSERNAME): $(LIBDIR)/lib$(APPUTILS).a ./$(APPSERNAME).c
	$(CC) $(CFLAGS) ./$(APPSERNAME).c $(LDFLAGS) -L$(LIBDIR) -l$(APPUTILS) -o ./bin/$(APPSERNAME)

.PHONY: $(APPPARNAME)
$(APPPARNAME): $(LIBDIR)/lib$(APPUTILS).a ./$(APPPARNAME).c
	mpicc $(CFLAGS) -no-pie ./$(APPPARNAME).c $(LDFLAGS) -L$(LIBDIR) -l$(APPUTILS) -o ./bin/$(APPPARNAME)

.PHONY: doc
doc: Doxyfile
	-doxygen Doxyfile
	-moxygen -g -a -o ./doc/jacobi-mpi-%s.md ./doc/xml/
	#-rm -r doc/xml/

clean:
	-rm bin/$(APPSERNAME)
	-rm bin/$(APPPARNAME)
	-rm -r doc/**
