#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>

#define SOCK_SERVICE_PORT  "9000"
//#define SOCK_DATA_FILE "/var/tmp/aesdsocketdata"
#define SOCK_DATA_FILE "/tmp/aesdsocketdata"

int get_sockaddr(struct sockaddr* saddr) {
    // struct addrinfo *ainfo;
    // struct addrinfo hints = {0};
    // hints.ai_family = AF_UNSPEC;
    // hints.ai_socktype = SOCK_STREAM;
    // hints.ai_flags = AI_PASSIVE;
    // int ret = getaddrinfo(NULL, SOCK_SERVICE_PORT, &hints, &ainfo);
    // if (ret < 0) {
    //     printf("getaddrinfo failed, ret=%d\n", ret);
    //     return -1;
    // }

    // memcpy(saddr, ainfo->ai_addr, sizeof(*saddr));
    // freeaddrinfo(ainfo);

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(9000);
    memcpy(saddr, &serv_addr, sizeof(*saddr));
    return 0;
}

int sock_bind_listen() 
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket failed. Error");
        return sockfd;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    struct sockaddr saddr = {0};
    int ret = get_sockaddr(&saddr);
    if (ret < 0) {
        perror("get_sockaddr failed. Error");
        goto out;
    }
    
    ret = bind(sockfd, &saddr, sizeof(saddr));
    if (ret < 0) {
        perror("bind failed. Error");
        goto out;
    }

    ret = listen(sockfd, 10);
    if (ret < 0) {
        printf("listen failed\n");
        goto out;
    }

    return sockfd;

out:
    close(sockfd);
    return ret;
}

int sock_receive(int connfd, int tmpdatafd) 
{
    char c = 0;
    // Read the data from socket to the TMP file
    printf("Recieving data...\n");
    while(c != '\n') {
        char data[255] = {0};
        int total_len = 0;
        int i = 0;
        // receive a packet ending with \n
        while(c != '\n' && total_len < 255) {
            int len = recv(connfd, &c, 1, 0);
            if (len <= 0) {
                if (len < 0) {
                    perror("recv failed. Error");
                }
                return -1;
            }
            data[i++] = c;
            total_len++;
        };

        //printf("Writing to tmp file, total len=%d\n", total_len);
        int len = write(tmpdatafd, &data, total_len);
        if (len <= 0) {
            if (len < 0) {
                perror("write failed. Error");
            }
            return -1;
        }
    }
    
    return 0;
}

int sock_send(int connfd, int tmpdatafd) 
{
    // Write data from TMP file to the socket
    lseek(tmpdatafd, 0L, SEEK_END);
    int total_len = lseek(tmpdatafd, 0, SEEK_CUR);
    char *buf = malloc(total_len);
    lseek(tmpdatafd, 0, SEEK_SET);

    int len = read(tmpdatafd, buf, total_len);
    if (len != total_len) {
        printf("read failed\n");
    }
    
    printf("Writing to sock, total len=%d\n", total_len);
    len = send(connfd, buf, total_len, 0);
    printf("sent %d bytes\n", len);
    if (len <= 0) {
        if (len < 0) {
            perror("send failed. Error");
        }
        return -1;
    }
    free(buf);
    return 0;
}

static volatile int terminate = 0; 

void signal_handler(int signo, siginfo_t *info, void *context) {
    printf("Caught signal, exiting\n");
    syslog(LOG_ERR, "Caught signal, exiting");
    terminate = 1;
}

int main(int argc, char *argv[])
{
    int opt;
    int deamonize = 0;

    while ((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
        case 'd': deamonize = 1; break;
        default:
            printf("Invalid options.");
            exit(EXIT_FAILURE);
        }
    }

    openlog(NULL, 0, LOG_USER);

    struct sigaction act = { 0 };
    act.sa_sigaction = &signal_handler;
    if (sigaction(SIGINT, &act, NULL) == -1) {
        perror("sigaction failed");
        return -1;
    }
    if (sigaction(SIGTERM, &act, NULL) == -1) {
        perror("sigaction failed");
        return -1;
    }

    int tmpdatafd = open(SOCK_DATA_FILE, O_APPEND | O_CREAT | O_RDWR, 0666);
    if (tmpdatafd < 0) { 
        printf("Opening %s failed, errno=%d\n", SOCK_DATA_FILE, errno);
        goto out;
    }

    int sockfd = sock_bind_listen();
    if (sockfd < 0) {
        printf("sock_bind_listen failed\n");
        goto out;
    }

    if (deamonize) {
        printf("Deamon mode\n");
        int ret = fork();
        if (ret < 0) {
            printf("fork failed\n");
            goto out;
        } else if (ret == 0) { // child
            printf("Child continues\n");
        } else { // parent
            printf("Parent exit, ret %d\n", ret);
            goto out;
            
        }
    }

    while(!terminate) {
        struct sockaddr addr = {0}; 
        socklen_t socklen = sizeof(addr);
        printf("Waiting for connections...\n");
        int connfd = accept(sockfd, &addr, &socklen);
        if (connfd < 0) return -1;

        syslog(LOG_ERR, "Accepted connection from 0x%x", addr.sa_data[0]);
        if (sock_receive(connfd, tmpdatafd) != 0) {
            close(connfd);
            break;
        }

        if (sock_send(connfd, tmpdatafd) != 0) {
            close(connfd);
            break;
        }

        close(connfd);
        syslog(LOG_ERR, "Closed connection from 0x%x", addr.sa_data[0]);
    }

out:
    if (sockfd) close(sockfd);
    if (tmpdatafd) close(tmpdatafd);
    remove(SOCK_DATA_FILE); 
    return 0; 
}