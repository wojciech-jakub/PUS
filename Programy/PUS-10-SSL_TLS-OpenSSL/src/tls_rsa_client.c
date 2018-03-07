/*
 * Data:                2009-05-25
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc tls_rsa_client.c -lssl -o tls_rsa_client
 * Uruchamianie:        $ ./tls_rsa_client <adres IP> <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_pton() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#define BUF_SIZE 1024

int verify_callback(int status, X509_STORE_CTX *store);

int main(int argc, char** argv) {

    int             sockfd;                 /* Desktryptor gniazda. */
    int             retval;                 /* Wartosc zwracana przez funkcje. */
    struct          sockaddr_in remote_addr;/* Gniazdowa struktura adresowa. */
    socklen_t       addr_len;               /* Rozmiar struktury w bajtach. */
    char            buff[BUF_SIZE];         /* Bufor dla funkcji read(). */
    int             bytes;                  /* Liczba bajtow. */
    int             total;


    SSL_CTX         *ctx;                   /* Kontekst SSL. */
    SSL             *ssl;

    if (argc != 3) {
        fprintf(stderr, "Invocation: %s <IPv4 ADDRESS> <PORT>\n", argv[0]);
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
    ctx = SSL_CTX_new(TLSv1_client_method());
    if (ctx == NULL) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    retval = SSL_CTX_load_verify_locations(ctx, NULL, "cert_dir");
    if (retval != 1) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verify_callback);

    /*
     * Utworzenie struktury SSL. Konfiguracja jest kopiowana z kontekstu
     * i moze zostac zmieniona. Podczas ustanawiania polaczenia uzywane sa
     * ustawienia ze struktury SSL.
     */
    ssl = SSL_new(ctx);
    if (ssl == NULL) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /* Utworzenie gniazda dla protokolu TCP: */
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    /*
     * Powiazanie struktury SSL z deskryptorem pliku. Tworzony jest "socket BIO" i
     * bedzie on posredniczyl w operacjach I/O miedzy ssl, a sockfd:
     */
    retval = SSL_set_fd(ssl, sockfd);
    if (retval == 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /* Wyzerowanie struktury adresowej dla adresu zdalnego (serwera): */
    memset(&remote_addr, 0, sizeof(remote_addr));

    /* Domena komunikacyjna (rodzina protokolow): */
    remote_addr.sin_family = AF_INET;

    /* Konwersja adresu IP z postaci czytelnej dla czlowieka: */
    retval = inet_pton(AF_INET, argv[1], &remote_addr.sin_addr);
    if (retval == 0) {
        fprintf(stderr, "inet_pton(): invalid network address!\n");
        exit(EXIT_FAILURE);
    } else if (retval == -1) {
        perror("inet_pton()");
        exit(EXIT_FAILURE);
    }

    remote_addr.sin_port = htons(atoi(argv[2])); /* Numer portu. */
    addr_len = sizeof(remote_addr); /* Rozmiar struktury adresowej w bajtach. */

    /* Nawiazanie polaczenia TCP (utworzenie asocjacji,
     * skojarzenie adresu zdalnego z gniazdem): */
    if (connect(sockfd, (const struct sockaddr*) &remote_addr, addr_len) == -1) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    /* Inicjacja polaczenia TLS (TLS handshake): */
    retval = SSL_connect(ssl);
    if (retval <= 0) {
        retval = SSL_get_error(ssl, retval);
        switch (retval) {
        case SSL_ERROR_ZERO_RETURN:
            fprintf(stderr, "SSL_connect(): closure alert!\n");
            exit(EXIT_FAILURE);
        case SSL_ERROR_SSL:
            fprintf(stderr, "SSL_connect(): SSL error!\n");
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "SSL_connect(): error (other)!\n");
            exit(EXIT_FAILURE);
        }
    }

    /* Odebranie danych: */
    bytes = 0;
    total = 0;
    while ((bytes = SSL_read(ssl, buff + total, BUF_SIZE - total)) > 0) {
        total += bytes;
    }

    if (bytes <= 0) {
        retval = SSL_get_error(ssl, bytes);
        switch (retval) {
        case SSL_ERROR_ZERO_RETURN:
            break;
        case SSL_ERROR_SSL:
            fprintf(stderr, "SSL_read(): SSL error!\n");
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "SSL_read(): error (default)!\n");
            exit(EXIT_FAILURE);
        }
    }

    fprintf(stdout, "Received message: ");
    fwrite(buff, sizeof(char), total, stdout);
    fprintf(stdout, "\n");

    close(sockfd);

    /* Zwolnienie struktury SSL: */
    SSL_free(ssl);
    /* Zwolnienie kontekstu: */
    SSL_CTX_free(ctx);
    /* Zwolnienie opisow bledow: */
    ERR_free_strings();

    exit(EXIT_SUCCESS);
}


int verify_callback(int status, X509_STORE_CTX *store) {

    char data[256];
    int depth;
    int err;

    if (!status) {
        X509 *cert = X509_STORE_CTX_get_current_cert(store);
        depth = X509_STORE_CTX_get_error_depth(store);
        err = X509_STORE_CTX_get_error(store);

        fprintf(stderr, "Error with certificate at depth: %d\n", depth);
        X509_NAME_oneline(X509_get_issuer_name(cert), data, 256);
        fprintf(stderr," ISSUER: %s\n", data);
        X509_NAME_oneline(X509_get_subject_name(cert), data, 256);
        fprintf(stderr," SUBJECT: %s\n", data);

        fprintf(stderr," ERROR: %d:%s\n", err,
                X509_verify_cert_error_string(err));
    }

    return status;
}
