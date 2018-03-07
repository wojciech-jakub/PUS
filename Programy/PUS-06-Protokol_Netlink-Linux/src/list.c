/*
 * Data:                2009-03-27
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc list.c -o list
 * Uruchamianie:        $ ./list <4 lub 6>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <asm/types.h>
#include <arpa/inet.h>
#include <net/if.h>

/* Struktura przechowujaca informacje na temat interfejsu: */
struct interface {
    char                    name[IF_NAMESIZE];
    int                     index;
    unsigned char           prefix;
    char                    address[INET6_ADDRSTRLEN];
    char                    broadcast[INET6_ADDRSTRLEN];
    char                    *scope;
};

#define BUFF_SIZE 16384

int main(int argc, char** argv) {

    int                     sockfd;
    int                     retval, bytes;
    char                    *recv_buff, *send_buff;
    struct msghdr           msg;    /* Struktura dla sendmsg() i recvmsg() */
    struct iovec            iov;    /* Dla msghdr */
    struct nlmsghdr         *nh;    /* Naglowek wysylanego komunikatu */
    struct nlmsghdr         *nh_tmp;
    struct sockaddr_nl      sa;     /* Struktura adresowa */
    struct ifaddrmsg        *ia;    /* Struktura zawierajaca inform. adresowe */
    struct rtattr           *attr;  /* Opcjonalne atrybuty */
    unsigned int            attr_len;
    unsigned int            sequence_number;
    unsigned int            msg_len;
    unsigned char           family; /* AF_INET or AF_INET6 */
    struct interface        in_info;

    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <IP VERSION>\n"
                "<IP VERSION> = 4 or 6\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    family = atoi(argv[1]);
    if (family == 4) {
        family = AF_INET;
    } else if (family == 6) {
        family = AF_INET6;
    } else {
        fprintf(stderr, "<IP VERSION> must be 4 or 6!\n");
        exit(EXIT_FAILURE);
    }

    /* Utworzenie gniazda: */
    sockfd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    /*
     * Gniazdo surowe z reguly wymaga, aby proces byl uprzywilejowany (EUID == 0)
     * lub posiadal CAP_NET_RAW capability. W przypadku PF_NETLINK nie jest to
     * konieczne.
     */


    /* Wyzerowanie struktury adresowej.
     * Jadro jest odpowiedzialne za ustalenie identyfikatora gniazda.
     * Ponieważ jest to pierwsze (i jedyne) gniazdo procesu,
     * identyfikatorem będzie PID procesu. */
    memset(&sa, 0, sizeof(struct sockaddr_nl));
    sa.nl_family            =       AF_NETLINK;

    if (bind(sockfd, (struct sockaddr*) &sa, sizeof(struct sockaddr_nl)) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    /*
     * Alokacja pamieci dla wysylanego komunikatu.
     *
     * NLMSG_SPACE(sizeof(struct ifaddrmsg)) = rozmiar naglowka 'nlmsghdr'
     * + opcjonalny padding + rozmiar naglowka 'ifaddrmsg' + opcjonalny padding.
     */
    send_buff = malloc(NLMSG_SPACE(sizeof(struct ifaddrmsg)));
    if (send_buff == NULL) {
        fprintf(stderr, "malloc() failed!\n");
        exit(EXIT_FAILURE);
    }


    sequence_number         =       0;

    /* Wypelnianie komunikatu do wysłania: */
    nh                      = (struct nlmsghdr*)send_buff;

    nh->nlmsg_len           =       NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    /* Typ - pobranie informacji adresowych: */
    nh->nlmsg_type          =       RTM_GETADDR;

    /* Pobranie informacji adresowych wszystkich interfejsow: */
    nh->nlmsg_flags         =       NLM_F_REQUEST | NLM_F_ROOT;

    nh->nlmsg_seq           =       ++sequence_number;
    nh->nlmsg_pid           =       getpid();


    iov.iov_base            =       send_buff;
    iov.iov_len             =       nh->nlmsg_len;


    msg.msg_name            = (void*)&sa;
    msg.msg_namelen         =       sizeof(struct sockaddr_nl);
    msg.msg_iov             =       &iov;
    msg.msg_iovlen          =       1;
    msg.msg_control         =       NULL;
    msg.msg_controllen      =       0;
    msg.msg_flags           =       0;

    ia                      = (struct ifaddrmsg*)NLMSG_DATA(nh);
    memset(ia, 0, sizeof(struct ifaddrmsg));
    /* Rodzina protokolow na podstawie argumentu wywolania: */
    ia->ifa_family          =       family;

    /* Struktura adresowa jest wyzerowana.
     * Wiadomosc bedzie przeznaczona do jadra (sa.nl_pid rowne 0). */
    memset(&sa, 0, sizeof(struct sockaddr_nl));
    sa.nl_family            =       AF_NETLINK;

    /* Wyslanie komunikatu do jadra: */
    retval = sendmsg(sockfd, &msg, 0);
    if (retval == -1) {
        perror("sendmsg()");
        exit(EXIT_FAILURE);
    }
    free(send_buff);

    /* Alokacja pamieci buforu dla danych odbieranych: */
    recv_buff = malloc(sizeof(char) * BUFF_SIZE);
    if (recv_buff == NULL) {
        fprintf(stderr, "malloc() failed!\n");
        exit(EXIT_FAILURE);
    }

    iov.iov_base            = (void*)recv_buff;
    iov.iov_len             =       BUFF_SIZE;

    /* Odebranie odpowiedzi: */
    msg_len                 =       0;
    for (;;) {

        bytes = recvmsg(sockfd, &msg, 0);
        if (retval == -1) {
            perror("recvmsg()");
            exit(EXIT_FAILURE);
        }

        msg_len                 +=      bytes;

        nh_tmp = (struct nlmsghdr*)msg.msg_iov->iov_base;
        if (nh_tmp->nlmsg_type == NLMSG_DONE) {
            break;
        }

        msg.msg_iov->iov_base   =       recv_buff + msg_len;
        msg.msg_iov->iov_len    -=      bytes;

    }

    /* Odpowiedz moze zawierac wiecej niz jeden naglowek. Iteracja: */
    nh = (struct nlmsghdr *) recv_buff;
    for (; NLMSG_OK(nh, msg_len); nh = NLMSG_NEXT(nh, msg_len)) {

        if (nh->nlmsg_type == NLMSG_DONE) {
            break;
        }

        if (nh->nlmsg_type == NLMSG_ERROR) {
            fprintf(stderr, "NLMSG_ERROR!\n");
            continue;
        }

        /* Gdy odebrano odpowiedz z nieprawidlowym numerem sekwencyjnym
         * (tzn. nie jest to odpowiedz na wyslany komunikat): */
        if (nh->nlmsg_pid != getpid() || nh->nlmsg_seq != sequence_number) {
            fprintf(stderr, "Invalid message!\n");
            continue;
        }

        memset(&in_info, 0, sizeof(struct interface));

        ia = (struct ifaddrmsg*)NLMSG_DATA(nh);

        /* W przypadku IPv6 w in_info.name zapamietujemy nazwe interfejsu: */
        if (ia->ifa_family == AF_INET6) {
            if (if_indextoname(ia->ifa_index, in_info.name) == NULL) {
                perror("if_indextoname()");
                exit(EXIT_FAILURE);
            }
        }

        in_info.index           =       ia->ifa_index;
        in_info.prefix          =       ia->ifa_prefixlen;

        /* Naglowek pierwszego atrybutu: */
        attr = (struct rtattr*)((char*)ia
                                + NLMSG_ALIGN(sizeof(struct ifaddrmsg)));

        attr_len = NLMSG_PAYLOAD(nh, sizeof(struct ifaddrmsg));

        /* Odpowiedz moze zawierac kilka atrybutow: */
        for (; RTA_OK(attr, attr_len); attr = RTA_NEXT(attr, attr_len)) {

            /* Sprawdzanie typu atrybutu: */
            if (attr->rta_type == IFA_LABEL) {

                strcpy(in_info.name, (char*)RTA_DATA(attr));

            } else if (attr->rta_type == IFA_ADDRESS) {

                inet_ntop(
                    family, RTA_DATA(attr),
                    in_info.address, INET6_ADDRSTRLEN
                );

            } else if (attr->rta_type == IFA_BROADCAST) {

                inet_ntop(
                    family, RTA_DATA(attr),
                    in_info.broadcast, INET6_ADDRSTRLEN
                );
            }

        } /* for */

        /*
         * cat /etc/iproute2/rt_scopes
         * lub
         * <linux/rtnetlink.h>: enum rt_scope_t
         */
        switch (ia->ifa_scope) {
        case RT_SCOPE_UNIVERSE:
            in_info.scope = "global";
            break;
        case RT_SCOPE_SITE:
            in_info.scope = "site";
            break;
        case RT_SCOPE_HOST:
            in_info.scope = "host";
            break;
        case RT_SCOPE_LINK:
            in_info.scope = "link";
            break;
        case RT_SCOPE_NOWHERE:
            in_info.scope = "nowhere";
            break;
        default:
            in_info.scope = "unknown";
        }

        /* Wypisanie informacji na tmat interfejsu: */
        fprintf(stdout, "Name: %s\n", in_info.name);
        fprintf(stdout, "Index: %d\n", in_info.index);
        fprintf(stdout, "Address: %s/%d\n", in_info.address, in_info.prefix);
        fprintf(stdout, "Scope: %s\n", in_info.scope);
        if (ia->ifa_family == AF_INET) {
            fprintf(stdout, "Broadcast: %s\n", in_info.broadcast);
        }
        fprintf(stdout, "\n");
    }

    free(recv_buff);
    close(sockfd);
    exit(EXIT_SUCCESS);
}


/*
struct msghdr {
    void *msg_name;             // Address to send to/receive from.
    socklen_t msg_namelen;      // Length of address data.

    struct iovec *msg_iov;      // Vector of data to send/receive into.
    size_t msg_iovlen;          // Number of elements in the vector.

    void *msg_control;          // Ancillary data (eg BSD filedesc passing).
    size_t msg_controllen;
                                // Ancillary data buffer length.
                                // !! The type should be socklen_t but the
                                // definition of the kernel is incompatible
                                // with this.

    int msg_flags;              // Flags on received message.
};
*/

/* Structure for scatter/gather I/O.  */
/*
struct iovec {
    void *iov_base;     // Pointer to data.
    size_t iov_len;     // Length of data.
};
*/

/*
struct nlmsghdr {
        __u32 nlmsg_len;    // Length of message including header.
        __u16 nlmsg_type;   // Type of message content.
        __u16 nlmsg_flags;  // Additional flags.
        __u32 nlmsg_seq;    // Sequence number.
        __u32 nlmsg_pid;    // PID of the sending process.
};
*/

/*
struct sockaddr_nl {
        sa_family_t     nl_family;  // AF_NETLINK
        unsigned short  nl_pad;     // Zero.
        pid_t           nl_pid;     // Process ID.
        __u32           nl_groups;  // Multicast groups mask.
};
*/

/*
struct ifaddrmsg {
        unsigned char ifa_family;    // Address type
        unsigned char ifa_prefixlen; // Prefixlength of address
        unsigned char ifa_flags;     // Address flags
        unsigned char ifa_scope;     // Address scope
        int           ifa_index;     // Interface index
};
*/

