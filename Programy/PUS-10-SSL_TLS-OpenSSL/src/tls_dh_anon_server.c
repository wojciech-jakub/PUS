/*
 * Data:                2009-05-25
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc tls_dh_anon_server.c -lssl -o tls_dh_anon_server
 * Uruchamianie:        $ ./tls_dh_anon_server <numer portu>
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

DH *get_DH_params(int keylength);
DH *tmp_dh_callback(SSL *ssl, int is_export, int keylength);


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
     * Zdefiniowanie listy zestawow algorytmow kryptograficznych.
     * ADH - anonimowy DH (bez uwierzytelniania); polaczenie podatne na ataki
     * man-in-the-middle.
     * HIGH - silne algorytmy kryptograficzne (klucz >= 128)
     */
    retval = SSL_CTX_set_cipher_list(ctx, "ADH+HIGH");
    if (retval == 0) {
        fprintf(stderr, "SSL_CTX_set_cipher_list() failed!\n");
        exit(EXIT_FAILURE);
    }

    /* Zdefiniowanie funkcji odpowiedzialnej za dostarczanie parametrow DH: */
    SSL_CTX_set_tmp_dh_callback(ctx, tmp_dh_callback);

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


/*
 * Funkcja odpowiedzialna za wczytanie parametrow DH z pliku DH_<keylength>.pem,
 * gdzie <keylength> jest dlugoscia liczby pierwszej "p" w bitach.
 */
DH *get_DH_params(int keylength) {

    DH *dh;
    FILE *file;
    char filename[256];

    /* Okreslenie nazwy pliku na podstawie dlugosci klucza: */
    sprintf(filename, "DH_%d.pem", keylength);

    file = fopen(filename, "r");
    if (file == NULL) {
        perror("fopen()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Reading DH params from %s...\n", filename);

    /*
     * Wczytanie parametrow DH (g, p).
     * Parametry "g" i "p" sa zawsze takie same, ale zmienia sie
     * prywatna wartosc "y" serwera.
     * Do klienta przesylane sa: "g", "p" oraz "g^y mod p".
     */
    dh = PEM_read_DHparams(file, NULL, NULL, NULL);
    if (dh == NULL) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return dh;
}


/*
 * Funkcja typu callback wywolywana przez proces, gdy wymagane sa parametry DH,
 * tzn. podczas ustanawiania polaczenia z klientem.
 */
DH *tmp_dh_callback(SSL *ssl, int is_export, int keylength) {

    static DH *dh_512  = NULL;
    static DH *dh_1024 = NULL;
    static DH *dh_2048 = NULL;
    static DH *dh_4096 = NULL;

    switch (keylength) {
    case 512:
        if (dh_512 == NULL) {
            dh_512 = get_DH_params(512);
        }
        return dh_512;
    case 1024:
        if (dh_1024 == NULL) {
            dh_1024 = get_DH_params(1024);
        }
        return dh_1024;
    case 2048:
        if (dh_2048 == NULL) {
            dh_2048 = get_DH_params(2048);
        }
        return dh_2048;
    case 4096:
        if (dh_4096 == NULL) {
            dh_4096 = get_DH_params(4096);
        }
        return dh_4096;
    default:
        fprintf(stderr, "No DH params for keylength: %d\n", keylength);
        exit(EXIT_FAILURE);
    }
}
