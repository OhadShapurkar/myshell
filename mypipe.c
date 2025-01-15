#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>



int main() {
    char buf[1024];
    pid_t pid;
    int fds[2];

    if (pipe(fds) == -1) {
        perror("pipe error");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork error ");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        close(fds[0]);
        write(fds[1], "Hello", 5);
        close(fds[1]);
    } 
    else 
    {
        close(fds[1]);
        read(fds[0], buf, sizeof(buf));
        printf("Received: %s\n", buf);
        close(fds[0]);   
    }

    return 0;
}