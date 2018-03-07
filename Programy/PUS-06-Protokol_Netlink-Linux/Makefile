PROJECT = 06
SYSTEM = linux
SRCDIR = src
BINDIR = bin
CC = gcc
CFLAGS = -O2 -pedantic -Wall

all: event ipaddr ipaddrmod list

clean:
	@rm -rf $(BINDIR)/*

distrib: clean
	tar -cvf $(SYSTEM)-$(PROJECT).tar $(SRCDIR)/* Makefile

event: event.o
	$(CC) $(CFLAGS) $(BINDIR)/$@.o -o $(BINDIR)/$@

event.o: $(SRCDIR)/event.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(basename $@).c -o $(BINDIR)/$(basename $@).o

ipaddr: ipaddr.o libnetlink.o
	$(CC) $(CFLAGS) $(BINDIR)/$@.o $(BINDIR)/libnetlink.o -o $(BINDIR)/$@

ipaddr.o: $(SRCDIR)/ipaddr.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(basename $@).c -o $(BINDIR)/$(basename $@).o

ipaddrmod: ipaddrmod.o libnetlink.o
	$(CC) $(CFLAGS) $(BINDIR)/$@.o $(BINDIR)/libnetlink.o -o $(BINDIR)/$@

ipaddrmod.o: $(SRCDIR)/ipaddrmod.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(basename $@).c -o $(BINDIR)/$(basename $@).o

libnetlink.o: $(SRCDIR)/libnetlink.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(basename $@).c -o $(BINDIR)/$(basename $@).o

list: list.o
	$(CC) $(CFLAGS) $(BINDIR)/$@.o -o $(BINDIR)/$@

list.o: $(SRCDIR)/list.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(basename $@).c -o $(BINDIR)/$(basename $@).o
