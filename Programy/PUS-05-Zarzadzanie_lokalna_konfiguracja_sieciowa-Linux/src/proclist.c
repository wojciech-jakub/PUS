/*
 * Data:                2009-03-15
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc proclist.c -o proclist
 * Uruchamianie:        $ ./proclist
 */

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <net/if.h>
#include <errno.h>

/* Z pliku: /usr/src/linux-<version>/include/net/ipv6.h */
#define IPV6_ADDR_ANY           0x0000U
#define IPV6_ADDR_LOOPBACK      0x0010U
#define IPV6_ADDR_LINKLOCAL     0x0020U
#define IPV6_ADDR_SITELOCAL     0x0040U
#define IPV6_ADDR_COMPATv4      0x0080U


int main(int argc, char** argv) {

    /* Adres w postaci czytelnej dla czlowieka: */
    char                    ipv6_address[INET6_ADDRSTRLEN];
    /* Nazwa interfejsu, np. eth0: */
    char                    interface_name[IF_NAMESIZE];

    unsigned int            prefix_len, scope, flags, interface_idx;
    char                    tmp[8][5];
    /* Wskaźnik na poczatek listy i na aktualnie przetwarzany element listy: */
    struct if_nameindex     *if_list, *if_ptr;

    FILE                    *file;


    /* Otwieramy do odczytu: */
    file = fopen("/proc/net/if_inet6", "r");
    if (file == NULL) {
        perror("fopen()");
        exit(EXIT_FAILURE);
    }

    /* Naglowek: */
    printf("   Name  Index\t\t  Address/prefix length\t\t\tScope\n");

    /* Wczytywanie linii w odpowiednim formacie.
     * Prosze porownac cat /proc/net/if_inet6. */
    while (fscanf(file, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %02x %02x %02x %20s\n",
                  tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5], tmp[6], tmp[7],
                  &interface_idx, &prefix_len, &scope, &flags, interface_name) != EOF) {

        sprintf(ipv6_address, "%s:%s:%s:%s:%s:%s:%s:%s",
                tmp[0], tmp[1], tmp[2], tmp[3],
                tmp[4], tmp[5], tmp[6], tmp[7]);

        printf(
            "%7s  %3d\t%s/%d\t",
            interface_name, interface_idx, ipv6_address, prefix_len
        );

        switch (scope) {
        case IPV6_ADDR_ANY:
            fprintf(stdout, "Global");
            break;
        case IPV6_ADDR_LINKLOCAL:
            fprintf(stdout, "Link");
            break;
        case IPV6_ADDR_SITELOCAL:
            fprintf(stdout, "Site");
            break;
        case IPV6_ADDR_COMPATv4:
            fprintf(stdout, "Compat");
            break;
        case IPV6_ADDR_LOOPBACK:
            fprintf(stdout, "Host");
            break;
        default:
            fprintf(stdout, "Unknown");
        }
        fprintf(stdout, "\n");
    }

    fclose(file);

    /*
     * Jezeli intrfejs jest w stanie DOWN, to nie znajdzie sie w /proc/net/if_inet6,
     * ale zostanie wypisany dzieki funkcji if_nameindex().
     */
    fprintf(stdout, "\nInterface list (if_nameindex):\n\n");

    /* Elementy listy zawieraja nazwe i indeks interfejsu: */
    if_list = if_nameindex();
    if (if_list == NULL) {
        perror("if_nameindex()");
        exit(EXIT_FAILURE);
    }

    /* Iterujemy przez elementy listy. Ostatni element to struktura
     * o wyzerowanych polach: */
    for (
        if_ptr = if_list;
        if_ptr->if_index != 0 && if_ptr->if_name != NULL;
        if_ptr++
    ) {

        fprintf(stdout, "%7s  %3d\n", if_ptr->if_name, if_ptr->if_index);
    }

    /* Zwalniamy pamiec zaalokowana dla listy: */
    if_freenameindex(if_list);

    exit(EXIT_SUCCESS);
}
