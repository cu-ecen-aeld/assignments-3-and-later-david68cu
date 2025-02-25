#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <syslog.h>

#define PORT 9000
#define BUFFER_SIZE 65536 // 64KB
#define LOG_FILE "/var/tmp/aesdsocketdata"

volatile sig_atomic_t shutdown_flag = 0;

void signal_handler(int sig) {
    shutdown_flag = 1;
    // This make the signal handler no reentrant
    //syslog(LOG_INFO, "Caught signal, exiting");
    // Correct way to log in signal handler
    // Use async-safe write() instead of syslog()
    const char msg[] = "Caught signal, exiting\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
}

static int cleanup_done = 0;

void cleanup(int *server_socket, int *client_socket, FILE **log_file) {
    if (cleanup_done) return;  // Ensure cleanup runs only once
    cleanup_done = 1;

    if (*log_file) {
        fflush(*log_file);
        fsync(fileno(*log_file));
        fclose(*log_file);
        *log_file = NULL;  // Prevent double fclose()
    }

    if (*client_socket > 0) {
        close(*client_socket);
        *client_socket = -1;
    }
    if (*server_socket > 0) {
        close(*server_socket);
        *server_socket = -1;
    }
    remove(LOG_FILE);
}


void setup_signal_handling() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

void daemonize() {
    pid_t pid;

    // Fork off the parent process
    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {  // Parent exits
        exit(EXIT_SUCCESS);
    }

    // Create a new session
    if (setsid() < 0) {
        perror("setsid failed");
        exit(EXIT_FAILURE);
    }

    // Fork again to prevent acquiring a terminal again
    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) { // First child exits
        exit(EXIT_SUCCESS);
    }

    // Set permissions and change working directory
    umask(0);
    if (chdir("/") < 0) {
        perror("chdir failed");
        exit(EXIT_FAILURE);
    }

    // Redirect standard file descriptors
    int fd = open("/dev/null", O_RDWR);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }
}


int main(int argc, char *argv[]) {
    openlog("aesdsocket", LOG_PID, LOG_USER);

    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    int daemon_mode = 0;
    int server_socket = -1, client_socket = -1;
    FILE *log_file = NULL;

    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = 1;
    }

    setup_signal_handling();

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        cleanup(&server_socket, &client_socket, &log_file);
        exit(1);
    }

    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        cleanup(&server_socket, &client_socket, &log_file);
        exit(1);
    }

    if (daemon_mode) {
        daemonize();
    }

    // Make server socket non-blocking
    int flags = fcntl(server_socket, F_GETFL, 0);
    if (flags < 0 || fcntl(server_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("Failed to make server socket non-blocking");
        cleanup(&server_socket, &client_socket, &log_file);
        exit(1);
    }

    fd_set read_fds;
    struct timeval timeout;

    while (!shutdown_flag) {
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(server_socket + 1, &read_fds, NULL, NULL, &timeout);
        if (activity < 0 && errno!= EINTR) {
            perror("Select error");
            break;
        }

        if (FD_ISSET(server_socket, &read_fds)) {
            client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
            if (client_socket < 0) {
                if (shutdown_flag) break;
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                perror("Accept failed");
                continue; 
            }

            syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));

            // Make client socket non-blocking. In Linux everything is a file including sockets
            // fcntl is crucial for making the sockets non-blocking
            // fcntl(F_GETFL) This command tells fcntl to retrieve the current file status flags 
            // associated with the given file descriptor (in our case, the socket)
            flags = fcntl(client_socket, F_GETFL, 0);
            if (flags < 0) {
                perror("fcntl(F_GETFL) failed");
                close(client_socket);
                continue;
            }
            if (fcntl(client_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
                perror("fcntl(F_SETFL) failed");
                close(client_socket);
                continue;
            }

            syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));

            log_file = fopen(LOG_FILE, "a+");
            if (!log_file) {
                perror("Failed to open log file");
                close(client_socket);
                syslog(LOG_ERR, "Failed to open log file for client %s", inet_ntoa(client_addr.sin_addr));
                continue;
            }

            memset(buffer, 0, BUFFER_SIZE);
            int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);

            if (bytes_received < 0) {
                if (shutdown_flag) break;
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                perror("recv failed");

                if (log_file) {
                    fclose(log_file);
                    log_file = NULL;
                }

                close(client_socket);
                continue;
            }

            for (int i = 0; i < bytes_received; i++) {
                fputc(buffer[i], log_file);
                if (buffer[i] == '\n') {
                    fflush(log_file);
                    fsync(fileno(log_file));

                    // Send the entire file back to the client
                    fseek(log_file, 0, SEEK_SET);
                    while (fgets(buffer, BUFFER_SIZE, log_file)) {
                        send(client_socket, buffer, strlen(buffer), 0);
                    }
                }
            }

            syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_addr.sin_addr));
            if (log_file) {  // Ensure it's not already closed
                fflush(log_file);
                fsync(fileno(log_file));
                fclose(log_file);
                log_file = NULL;  // Prevent double fclose()
            }
            close(client_socket);
        }
    }

    cleanup(&server_socket, &client_socket, &log_file);
    closelog();
    return 0;
}