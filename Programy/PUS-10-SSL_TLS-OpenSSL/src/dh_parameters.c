/*
 * Data:                2009-05-25
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc dh_parameters.c -o dh_parameters
 * Uruchamianie:        $ ./dh_parameters <length>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/dh.h>
#include <openssl/pem.h>

void callback(int p, int n, void *arg) {

    int retval;
    char c = '?';

    if (p == 0) /* Wygenerowanie potencjalnej liczby pierwszej. */
        c = '.';
    if (p == 1) /* Test pierwszosci. */
        c = '+';
    if (p == 2) /* Odnaleziono liczbe pierwsza. */
        c = '*';
    if (p == 3) /* Odnaleziono liczbe pierwsza. */
        c = '\n';

    /* Zapis znaku bez buforowania na standardowe wyjscie: */
    retval = write(STDOUT_FILENO, &c, 1);
}

DH* generate_dh_parameters(int length) {

    int codes;
    DH *dh;

    for (;;) {
        dh = DH_generate_parameters(length, DH_GENERATOR_5, callback, NULL);
        if (dh == NULL) {
            ERR_print_errors_fp(stderr);
            exit(EXIT_FAILURE);
        }

        /* Weryfikacja parametrow: */
        DH_check(dh, &codes);
        switch (codes) {
        case DH_CHECK_P_NOT_SAFE_PRIME:
        case DH_NOT_SUITABLE_GENERATOR:
        case DH_UNABLE_TO_CHECK_GENERATOR:
            fprintf(stdout, "DH parameters are bad.");
            DH_free(dh);
            break;
        default:
            return dh;
        }
    }
}

int main(int argc, char** argv) {

    DH      *dh;
    FILE    *file;
    char    filename[256];
    int     length;

    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <LENGTH>\n", argv[0]);
        exit(EXIT_FAILURE);
    }


    /* Wczytanie tekstowych opisow bledow: */
    ERR_load_crypto_strings();

    /*
     * Inicjalizacja generatora liczb pseudolosowych za pomoca pliku
     * /dev/random ("very high quality randomness"):
     * Operacja blokujaca (jezeli nie ma wystarczajacej ilosci bajtow).
     *
     * /dev/random  gathers environmental noise from device drivers and other
     * sources into an entropy  pool. When the entropy pool is empty, reads from
     * /dev/random will block until additional environmental  noise.
     */
    fprintf(stdout, "Reading 32 bytes of entropy from /dev/random... ");
    fflush(stdout);
    RAND_load_file("/dev/random", 32);
    fprintf(stdout, "OK\n");

    /* Wygenerowanie parametrow DH: */
    length = atoi(argv[1]);
    dh = generate_dh_parameters(length);

    /* Zapis parametrow do pliku: */
    sprintf(filename, "DH_%d.pem", length);
    file = fopen(filename, "w");
    if (file == NULL) {
        perror("fopen()");
        exit(EXIT_FAILURE);
    }

    /* Zapis parametrow do pliku w formacie PEM: */
    if (!PEM_write_DHparams(file, dh)) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (fclose(file) == EOF) {
        perror("fclose()");
        exit(EXIT_FAILURE);
    }

    DH_free(dh);

    /* Zwolnienie opisow bledow: */
    ERR_free_strings();

    exit(EXIT_SUCCESS);
}
