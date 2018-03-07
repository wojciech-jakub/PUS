/*
 * Data:                2009-05-25
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc tls_rsa_server.c -lssl -o tls_rsa_server
 * Uruchamianie:        $ ./tls_rsa_server <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntop() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <time.h>
#include <errno.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

int main(int argc, char** argv) {

    /* Deskryptory dla gniazda nasluchujacego i polaczonego: */
    int             listenfd, connfd;

    int             retval; /* Wartosc zwracana przez funkcje. */

    int             option;

    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct          sockaddr_in client_addr, server_addr;

    /* Rozmiar struktur w bajtach: */
    socklen_t       client_addr_len, server_addr_len;

    /* Bufor dla adresu IP klienta w postaci kropkowo-dziesietnej: */
    char            addr_buff[256];

    SSL_CTX         *ctx;                   /* Kontekst SSL. */
    SSL             *ssl;

    /* Wiadomosc do wyslania: */
    char*           message = "Laboratorium PUS.";

    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Wczytanie tekstowych opisow bledow: */
    SSL_load_error_strings();

    /* Inicjalizacja biblioteki - wczytanie nazw algorytmow szyfrujacych
     * i funkcji skrotu: */
    SSL_library_init();

    /*
     * Inicjalizacja generatora liczb pseudolosowych za pomoca pliku
     * /dev/urandom:
     */
    RAND_load_file("/dev/urandom", 128);

    /*
     * Utworzenie kontekstu dla protokolu TLSv1. Lista szyfrow, klucze, certyfikaty,
     * opcje, itd. sa inicjalizowane domyslnymi wartosciami. Podczas tworzenia
     * kontekstu podawana jest metoda polaczenia. Metoda okresla protokol oraz
     * przeznaczenie kontekstu (kontekst dla klienta, serwera lub kontekst ogolny).
     */
    ctx = SSL_CTX_new(TLSv1_server_method());
    if (ctx == NULL) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /*
     * Zdefiniowanie listy zestawow algorytmow kryptograficznych:
     * RSA - wymiana kluczy
     * HIGH - tylko silne algorytmy
     * RSA+HIGH - RSA and HIGH
     * !NULL - wykluczenie zestawow bez szyfrowania
     * Opis: man (1ssl) ciphers
     */
    retval = SSL_CTX_set_cipher_list(ctx, "RSA+HIGH:!NULL");
    if (retval == 0) {
        fprintf(stderr, "SSL_CTX_set_cipher_list() failed!\n");
        exit(EXIT_FAILURE);
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    retval = SSL_CTX_use_certificate_chain_file(ctx, "server_chain.pem");
    if (retval != 1) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    retval = SSL_CTX_use_PrivateKey_file(ctx, "server_keypair.pem", SSL_FILETYPE_PEM);
    if (retval != 1) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /* Utworzenie gniazda dla protokolu TCP: */
    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    /*
     * Serwer jest strona wykonujaca active close. Gniazdo polaczone po zamknieciu
     * jest w stanie TIME_WAIT (przez ok. 1 minute) i uniemozliwia
     * powiazanie gniazda nasluchujacego z numerem portu.
     * Ponizsza opcja pozwala na ponowne uzycie numeru portu.
     */
    retval = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));
    if (retval == -1) {
        perror("setsockopt()");
        exit(EXIT_FAILURE);
    }

    /* Wyzerowanie struktury adresowej serwera: */
    memset(&server_addr, 0, sizeof(server_addr));
    /* Domena komunikacyjna (rodzina protokolow): */
    server_addr.sin_family          =       AF_INET;
    /* Adres nieokreslony (ang. wildcard address): */
    server_addr.sin_addr.s_addr     =       htonl(INADDR_ANY);
    /* Numer portu: */
    server_addr.sin_port            =       htons(atoi(argv[1]));
    /* Rozmiar struktury adresowej serwera w bajtach: */
    server_addr_len                 =       sizeof(server_addr);

    /* Powiazanie "nazwy" (adresu IP i numeru portu) z gniazdem: */
    if (bind(listenfd, (struct sockaddr*) &server_addr, server_addr_len) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    /* Przeksztalcenie gniazda w gniazdo nasluchujace: */
    if (listen(listenfd, 2) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server is listening for incoming connection...\n");


    for (;;) {
        /* Funkcja pobiera polaczenie TCP z kolejki polaczen oczekujacych na
         * zaakceptowanie i zwraca deskryptor dla gniazda polaczonego: */
        client_addr_len = sizeof(client_addr);
        connfd = accept(listenfd,
                        (struct sockaddr*)&client_addr, &client_addr_len);

        if (connfd == -1) {
            perror("accept()");
            exit(EXIT_FAILURE);
        }

        fprintf(
            stdout, "TCP connection accepted from %s:%d\n",
            inet_ntop(AF_INET, &client_addr.sin_addr,
                      addr_buff, sizeof(addr_buff)),
            ntohs(client_addr.sin_port)
        );

        /*
         * Utworzenie struktury SSL. Konfiguracja jest kopiowana z kontekstu
         * i moze zostac zmieniona per-polaczenie.
         */
        ssl = SSL_new(ctx);
        if (ssl == NULL) {
            ERR_print_errors_fp(stderr);
            exit(EXIT_FAILURE);
        }

        /*
         * Powiazanie struktury SSL z deskryptorem pliku. Tworzony jest
         * "socket BIO" i bedzie on posredniczyl w operacjach I/O
         * miedzy ssl, a connfd:
         */
        retval = SSL_set_fd(ssl, connfd);
        if (retval == 0) {
            ERR_print_errors_fp(stderr);
            exit(EXIT_FAILURE);
        }

        /* Oczekiwanie na inicjacje polaczenia TLS: */
        retval = SSL_accept(ssl);
        if (retval <= 0) {
            retval = SSL_get_error(ssl, retval);
            switch (retval) {
            case SSL_ERROR_ZERO_RETURN:
                fprintf(stderr, "SSL_accept(): closure alert!\n");
                exit(EXIT_FAILURE);
            case SSL_ERROR_SSL:
                fprintf(stderr, "SSL_accept(): SSL error!\n");
                exit(EXIT_FAILURE);
            default:
                fprintf(stderr, "SSL_accept(): error (other)!\n");
                exit(EXIT_FAILURE);
            }
        }

        /* Wyslanie aktualnego czasu do klienta: */
        retval = SSL_write(ssl, message, strlen(message));
        if (retval <= 0) {
            retval = SSL_get_error(ssl, retval);
            switch (retval) {
            case SSL_ERROR_ZERO_RETURN:
                fprintf(stderr, "SSL_write(): closure alert!\n");
                exit(EXIT_FAILURE);
            case SSL_ERROR_SSL:
                fprintf(stderr, "SSL_write(): SSL error!\n");
                exit(EXIT_FAILURE);
            default:
                fprintf(stderr, "SSL_write(): error (other)!\n");
                exit(EXIT_FAILURE);
            }
        }

        /* Zamkniecie polaczenia TLS. Serwer wysyla do klienta wiadomosc Alert: */
        retval = SSL_shutdown(ssl);
        if (retval < 0) {
            switch (retval) {
            case SSL_ERROR_SSL:
                fprintf(stderr, "SSL_shutdown(): SSL error!\n");
                exit(EXIT_FAILURE);
            default:
                fprintf(stderr, "SSL_shutdown(): error (other)!\n");
                exit(EXIT_FAILURE);
            }
        }

        SSL_free(ssl);

        /* Zamkniecie polaczenia TCP. Serwer wykonuje active close: */
        close(connfd);
    }

}
