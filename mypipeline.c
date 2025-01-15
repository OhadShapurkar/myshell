#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {
    int fds[2];
    pid_t child1, child2;

    if (pipe(fds) == -1) {
        perror("pipe error");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>forking...)\n");
    child1 = fork();
    if (child1 < 0) {
        perror("fork error");
        exit(EXIT_FAILURE);
    }

    if (child1 == 0) {
        fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe...)\n");
        close(fds[0]); 
        dup2(fds[1], STDOUT_FILENO);
        close(fds[1]);  
        fprintf(stderr, "(child1>going to execute cmd: ls -l)\n");
        execlp("ls", "ls", "-l", NULL);  
        perror("execlp error");  
        exit(EXIT_FAILURE);
    } else {
        fprintf(stderr, "(parent_process>created process with id: %d)\n", child1);
        fprintf(stderr, "(parent_process>closing the write end of the pipe...)\n");
        close(fds[1]); 
    }

    fprintf(stderr, "(parent_process>forking...)\n");
    child2 = fork();
    if (child2 < 0) {
        perror("fork error");
        exit(EXIT_FAILURE);
    }

    if (child2 == 0) {
        fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe...)\n");
        close(fds[1]); 
        dup2(fds[0], STDIN_FILENO);  
        close(fds[0]);  
        fprintf(stderr, "(child2>going to execute cmd: tail -n 2)\n");
        execlp("tail", "tail", "-n", "2", NULL);  
        perror("execlp error");  
        exit(EXIT_FAILURE);
    } else {
        fprintf(stderr, "(parent_process>created process with id: %d)\n", child2);
        fprintf(stderr, "(parent_process>closing the read end of the pipe...)\n");
        close(fds[0]);
    }

    fprintf(stderr, "(parent_process>waiting for child processes to terminate...)\n");
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    fprintf(stderr, "(parent_process>exiting...)\n");
    return 0;
}
