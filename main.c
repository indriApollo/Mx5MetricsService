#include <stdio.h>
#include "stnobd.h"
#include "server.h"
#include "metrics.h"
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/mman.h>
#include <fcntl.h>

#define SERIAL_PORT_NAME   "/dev/pts/3"
#define SERIAL_BAUD_RATE    921600
#define SOCKET_NAME        "/tmp/mx5metrics.sock"
#define SHM_NAME           "/mx5metrics"
#define EPOLL_SINGLE_EVENT 1

static struct metrics* setup_shm() {
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0755);
    if (fd < 0) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, sizeof(struct metrics)) < 0) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    struct metrics *metrics = mmap(NULL, sizeof(struct metrics), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (metrics == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    close(fd);

    return metrics;
}

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

static void epoll_add_fd(int epfd, int fd) {
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fd;

    if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) < 0) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
}

static int setup_epoll(int signalfd_fd, int stnobd_fd, int socket_fd) {
    int fd = epoll_create1(0);
    if (fd < 0) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    epoll_add_fd(fd, signalfd_fd);
    epoll_add_fd(fd, stnobd_fd);
    epoll_add_fd(fd, socket_fd);

    return fd;
}

int main(void) {
    struct stnobd_context stnobd_context;

    struct metrics *metrics = setup_shm();

    int signalfd_fd = setup_signal_handler();

    printf("Setting up serial port %s\n", SERIAL_PORT_NAME);

    char *cfg_cmds[] = {
        STNOBD_CFG_DISABLE_ECHO,
        STNOBD_CFG_ENABLE_HEADER,
        STNOBD_CFG_DISABLE_SPACES,
        STNOBD_CFG_FILTER(CAN_ID_HEX_STR_BRAKES),
        STNOBD_CFG_FILTER(CAN_ID_HEX_STR_RPM_SPEED_ACCEL),
        STNOBD_CFG_FILTER(CAN_ID_HEX_STR_COOLANT_THROTTLE_INTAKE),
        STNOBD_CFG_FILTER(CAN_ID_HEX_STR_FUEL_LEVEL),
        STNOBD_CFG_FILTER(CAN_ID_HEX_STR_WHEEL_SPEEDS)
    };
    int cfg_cmds_count = sizeof(cfg_cmds) / sizeof(cfg_cmds[0]);

    int stnobd_fd = setup_stnobd(SERIAL_PORT_NAME, SERIAL_BAUD_RATE, cfg_cmds, cfg_cmds_count, &stnobd_context);
    if (stnobd_fd < 0) exit(EXIT_FAILURE);

    send_stnobd_reset_cmd(&stnobd_context);

    int socket_fd = setup_server_socket(SOCKET_NAME);
    if (socket_fd < 0) exit(EXIT_FAILURE);

    int epoll_fd = setup_epoll(signalfd_fd, stnobd_fd, socket_fd);

    struct epoll_event epoll_events[EPOLL_SINGLE_EVENT];

    printf("Ready at %s, /dev/shm%s\n", SOCKET_NAME, SHM_NAME);

    while(1) {
        if (epoll_wait(epoll_fd, epoll_events, EPOLL_SINGLE_EVENT, -1) != EPOLL_SINGLE_EVENT) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        if (!(epoll_events[0].events & EPOLLIN)) {
            fprintf(stderr, "Expected EPOLLIN, got %d\n", epoll_events[0].events);
            break;
        }

        if (epoll_events[0].data.fd == stnobd_fd) {
            handle_incoming_stnobd_msg(&stnobd_context, metrics);
        }
        else if (epoll_events[0].data.fd == socket_fd) {
            handle_incoming_server_msg(socket_fd, metrics);
        }
        else if (epoll_events[0].data.fd == signalfd_fd) {
            handle_signal(signalfd_fd);
            break;
        }
        else {
            fprintf(stderr, "Unexpected epoll event fd %d\n", epoll_events[0].data.fd);
            break;
        }
    }

    printf("Shutting down ....\n");

    close(epoll_fd);
    close(signalfd_fd);
    close_stnobd(&stnobd_context);
    close_server_socket(socket_fd, SOCKET_NAME);
    shm_unlink(SHM_NAME);

    printf("Bye :)\n");
    return 0;
}
