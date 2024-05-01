#include <stdio.h>
#include "server.h"
#include "metrics.h"
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>

#define SOCKET_NAME "/tmp/mx5metrics.sock"
#define EPOLL_SINGLE_EVENT 1

static int setup_signal_handler() {
    int fd;
    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    // Block signals so that they aren't handled
    // according to their default dispositions
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    fd = signalfd(-1, &mask, 0);
    if (fd == -1) {
        perror("signalfd");
        exit(EXIT_FAILURE);
    }

    return fd;
}

static void handle_signal(int fd) {
    struct signalfd_siginfo siginfo;

    ssize_t s = read(fd, &siginfo, sizeof(siginfo));
    if (s != sizeof(siginfo)) {
        perror("read signalfd");
        exit(EXIT_FAILURE);
    }

    if (siginfo.ssi_signo == SIGINT) {
        printf("Got SIGINT\n");
    } else if (siginfo.ssi_signo == SIGTERM) {
        printf("Got SIGTERM\n");
    } else {
        printf("Unexpected signal %d\n", siginfo.ssi_signo);
    }
}

static int setup_epoll(int signalfd_fd, int socket_fd) {
    int fd = epoll_create1(0);
    if (fd < 0) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event signalfd_epoll_event;
    signalfd_epoll_event.events = EPOLLIN;
    signalfd_epoll_event.data.fd = signalfd_fd;

    if(epoll_ctl(fd, EPOLL_CTL_ADD, signalfd_fd, &signalfd_epoll_event) < 0) {
        perror("epoll_ctl signalfd_fd");
        exit(EXIT_FAILURE);
    }

    struct epoll_event socket_epoll_event;
    socket_epoll_event.events = EPOLLIN;
    socket_epoll_event.data.fd = socket_fd;

    if(epoll_ctl(fd, EPOLL_CTL_ADD, socket_fd, &socket_epoll_event) < 0) {
        perror("epoll_ctl socket_fd");
        exit(EXIT_FAILURE);
    }

    return fd;
}

int main(void) {
    int signalfd_fd = setup_signal_handler();

    struct metrics metrics;
    metrics.rpm = 123;

    int socket_fd = setup_server_socket(SOCKET_NAME);
    if (socket_fd < 0) exit(EXIT_FAILURE);

    int epoll_fd = setup_epoll(signalfd_fd, socket_fd);

    struct epoll_event epoll_events[EPOLL_SINGLE_EVENT];

    printf("Ready at %s\n", SOCKET_NAME);

    while(1) {
        if (epoll_wait(epoll_fd, epoll_events, EPOLL_SINGLE_EVENT, -1) != EPOLL_SINGLE_EVENT) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        if (!(epoll_events[0].events & EPOLLIN)) {
            fprintf(stderr, "Expected EPOLLIN, got %d\n", epoll_events[0].events);
            break;
        }

        if (epoll_events[0].data.fd == signalfd_fd) {
            handle_signal(signalfd_fd);
            break;
        }
        else if (epoll_events[0].data.fd == socket_fd) {
            handle_incoming_server_msg(socket_fd, &metrics);
        }
        else {
            fprintf(stderr, "Unexpected epoll event fd %d\n", epoll_events[0].data.fd);
            break;
        }
    }

    printf("Shutting down ....\n");

    close(signalfd_fd);
    close(socket_fd);
    close(epoll_fd);

    unlink(SOCKET_NAME);

    printf("Bye :)\n");
    return 0;
}
