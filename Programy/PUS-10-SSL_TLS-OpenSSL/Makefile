PROJECT = 10
SYSTEM = linux
SRCDIR = src
BINDIR = bin
CC = gcc
CFLAGS = -O2 -pedantic -Wall

all: tcp_client tcp_server tls_dh_anon_client tls_dh_anon_server dh_parameters \
     tls_rsa_client tls_rsa_server tls_dhe_rsa_client tls_dhe_rsa_server \
     tls_auth_client tls_auth_server

files:
	@cp $(SRCDIR)/files/* $(BINDIR)/

clean:
	@rm -rf $(BINDIR)/*

distrib: clean
	tar -cvf $(SYSTEM)-$(PROJECT).tar $(SRCDIR)/* Makefile

tcp_client: tcp_client.o
	$(CC) $(CFLAGS) $(BINDIR)/$@.o -o $(BINDIR)/$@

tcp_client.o: $(SRCDIR)/tcp_client.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(basename $@).c -o $(BINDIR)/$(basename $@).o

tcp_server: tcp_server.o
	$(CC) $(CFLAGS) $(BINDIR)/$@.o -o $(BINDIR)/$@

tcp_server.o: $(SRCDIR)/tcp_server.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(basename $@).c -o $(BINDIR)/$(basename $@).o

tls_dh_anon_client: tls_dh_anon_client.o
	$(CC) $(CFLAGS) $(BINDIR)/$@.o -lssl -o $(BINDIR)/$@

tls_dh_anon_client.o: $(SRCDIR)/tls_dh_anon_client.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(basename $@).c -o $(BINDIR)/$(basename $@).o

tls_dh_anon_server: tls_dh_anon_server.o
	$(CC) $(CFLAGS) $(BINDIR)/$@.o -lssl -o $(BINDIR)/$@

tls_dh_anon_server.o: $(SRCDIR)/tls_dh_anon_server.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(basename $@).c -o $(BINDIR)/$(basename $@).o

dh_parameters: dh_parameters.o
	$(CC) $(CFLAGS) $(BINDIR)/$@.o -lcrypto -o $(BINDIR)/$@

dh_parameters.o: $(SRCDIR)/dh_parameters.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(basename $@).c -o $(BINDIR)/$(basename $@).o

tls_rsa_client: tls_rsa_client.o
	$(CC) $(CFLAGS) $(BINDIR)/$@.o -lssl -o $(BINDIR)/$@

tls_rsa_client.o: $(SRCDIR)/tls_rsa_client.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(basename $@).c -o $(BINDIR)/$(basename $@).o

tls_rsa_server: tls_rsa_server.o
	$(CC) $(CFLAGS) $(BINDIR)/$@.o -lssl -o $(BINDIR)/$@

tls_rsa_server.o: $(SRCDIR)/tls_rsa_server.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(basename $@).c -o $(BINDIR)/$(basename $@).o

tls_dhe_rsa_client: tls_dhe_rsa_client.o
	$(CC) $(CFLAGS) $(BINDIR)/$@.o -lssl -o $(BINDIR)/$@

tls_dhe_rsa_client.o: $(SRCDIR)/tls_dhe_rsa_client.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(basename $@).c -o $(BINDIR)/$(basename $@).o

tls_dhe_rsa_server: tls_dhe_rsa_server.o
	$(CC) $(CFLAGS) $(BINDIR)/$@.o -lssl -o $(BINDIR)/$@

tls_dhe_rsa_server.o: $(SRCDIR)/tls_dhe_rsa_server.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(basename $@).c -o $(BINDIR)/$(basename $@).o

tls_auth_client: tls_auth_client.o
	$(CC) $(CFLAGS) $(BINDIR)/$@.o -lssl -o $(BINDIR)/$@

tls_auth_client.o: $(SRCDIR)/tls_auth_client.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(basename $@).c -o $(BINDIR)/$(basename $@).o

tls_auth_server: tls_auth_server.o
	$(CC) $(CFLAGS) $(BINDIR)/$@.o -lssl -o $(BINDIR)/$@

tls_auth_server.o: $(SRCDIR)/tls_auth_server.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(basename $@).c -o $(BINDIR)/$(basename $@).o
