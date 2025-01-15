#include "LineParser.h"
#include <linux/limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h> 

#define TERMINATED -1
#define RUNNING 1
#define SUSPENDED 0
#define HISTLEN 10



typedef struct process
{
    cmdLine *cmd;
    char* name;
    pid_t pid;
    int status;
    struct process *next;
} process;

typedef struct history {
    char *command;
    struct history *next;
} history;


process *process_list = NULL;

int debug = 0;


int strlen2(const char* str)
{
    int i = 0;
    while(str[i] != '\0')
    {
        i++;
    }
    return i;
}

char* strcpy2(char* dest, const char* src)
{
    int i = 0;
    while(src[i] != '\0')
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}


void addToHistory(history **history_list, const char *command) {
    history *new_entry = malloc(sizeof(history));
    new_entry->command = strdup(command);
    new_entry->next = NULL;

    if (*history_list == NULL) {
        *history_list = new_entry;
    } else {
        history *current = *history_list, *prev = NULL;
        int count = 0;

        while (current && count < HISTLEN - 1) {
            prev = current;
            current = current->next;
            count++;
        }

        if (count == HISTLEN - 1 && prev) {
            free(prev->next->command);
            free(prev->next);
            prev->next = NULL;
        }

        new_entry->next = *history_list;
        *history_list = new_entry;
    }
}

void printHistory(history *history_list) {
    int index = 1;
    while (history_list) {
        printf("%d: %s\n", index++, history_list->command);
        history_list = history_list->next;
    }
}

// Execute commands from history
void executeHistory(history *history_list, int index, char *buf) {
    int current_index = 1;
    history *current = history_list;

    while (current && current_index < index) {
        current = current->next;
        current_index++;
    }

    if (current) {
        strcpy(buf, current->command);
    } else {
        fprintf(stderr, "No such history entry\n");
    }
}

void addProcess(process **process_list, cmdLine *cmd, pid_t pid)
{
    process *newProcess = (process *)malloc(sizeof(process));
    newProcess->name = malloc(strlen2(cmd->arguments[0]) + 1);
    strcpy2(newProcess->name, cmd->arguments[0]);
    newProcess->cmd = cmd;    
    newProcess->pid = pid;
    newProcess->status = RUNNING;
    newProcess->next = *process_list;
    *process_list = newProcess;
}

void printProcessList(process **process_list) {
    process *current = *process_list;
    printf("PID\t\tCommand\t\tSTATUS\n");

    while (current) {
        const char *status_str = 
            current->status == RUNNING ? "Running" : 
            current->status == SUSPENDED ? "Suspended" : 
            "Terminated";
        printf("%d\t\t%s\t\t%s\n", current->pid, current->name, status_str);
        current = current->next;
    }
}

void freeProcessList(process *process_list)
{
    process *curr = process_list;
    process *next;
    while(curr != NULL)
    {
        next = curr->next;
        //freeCmdLines(curr->cmd);
        free(curr->name);
        free(curr);
        curr = next;
    }
}

void updateProcessList(process **process_list)
{
    process *curr = *process_list;
    int status;
    pid_t pid;
    while(curr != NULL)
    {
        pid = waitpid(curr->pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        if(pid == -1)
        {
            curr->status = TERMINATED;
        }
        else if(pid == 0)
        {
            curr->status = RUNNING;
        }
        else
        {
            if(WIFEXITED(status) || WIFSIGNALED(status))
            {
                curr->status = TERMINATED;
            }
            else if(WIFSTOPPED(status))
            {
                curr->status = SUSPENDED;
            }
            else if(WIFCONTINUED(status))
            {
                curr->status = RUNNING;
            }
        }
        curr = curr->next;
    }
}

void updateProcessStatus(process **process_list, int pid, int status)
{
    process *curr = *process_list;
    while(curr != NULL)
    {
        if(curr->pid == pid)
        {
            curr->status = status;
            break;
        }
        curr = curr->next;
    }
}

void printProcessListUpdated(process **process_list) {
    updateProcessList(process_list);
    printProcessList(process_list);
}



void handleDirections(cmdLine *pCmdLine)
{
    if(pCmdLine->inputRedirect != NULL)
    {
        if(debug)
            fprintf(stderr, "Redirecting input to: %s\n", pCmdLine->inputRedirect);
        close(STDIN_FILENO);
        if(fopen(pCmdLine->inputRedirect, "r") == NULL)
        {
            perror("Error opening input file");
            exit(EXIT_FAILURE);
        }
    }
    if(pCmdLine->outputRedirect != NULL)
    {
        if(debug)
            fprintf(stderr, "Redirecting output to: %s\n", pCmdLine->outputRedirect);
        close(STDOUT_FILENO);
        if(fopen(pCmdLine->outputRedirect, "w") == NULL)
        {
            perror("Error opening output file");
            exit(EXIT_FAILURE);
        }
    }
}


void handleSignals(process *process_list, const char *command, pid_t pid) {
    int signal;

    if (strcmp(command, "stop") == 0) {
        signal = SIGTSTP;
    } else if (strcmp(command, "term") == 0) {
        signal = SIGINT;
    } else if (strcmp(command, "wake") == 0) {
        signal = SIGCONT;
    } else {
        fprintf(stderr, "Unknown command\n");
        return;
    }

    if (kill(pid, signal) == 0) {
        if (strcmp(command, "stop") == 0) {
            updateProcessStatus(&process_list, pid, SUSPENDED);
        } else if (strcmp(command, "term") == 0) {
            updateProcessStatus(&process_list, pid, TERMINATED);
        } else if (strcmp(command, "wake") == 0) {
            updateProcessStatus(&process_list, pid, RUNNING);
        }
    } else {
        perror("Error sending signal");
    }
}


void executePipe(cmdLine *pCmdLine) {
    int numPipes = 0;
    cmdLine *currentCmd = pCmdLine;
    
    // Count the number of pipes
    while (currentCmd->next != NULL) {
        numPipes++;
        currentCmd = currentCmd->next;
    }

    int pipefds[2 * numPipes];
    for (int i = 0; i < numPipes; i++) {
        if (pipe(pipefds + 2 * i) == -1) {
            perror("Error creating pipe");
            exit(EXIT_FAILURE);
        }
    }

    int i = 0;
    currentCmd = pCmdLine;
    while (currentCmd != NULL) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Error forking process");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) 
        { 
            if (i > 0) 
            { 
                if (dup2(pipefds[(i - 1) * 2], STDIN_FILENO) == -1) {
                    perror("Error redirecting input");
                    exit(EXIT_FAILURE);
                }
            }

            if (currentCmd->next != NULL) 
            { 
                if (dup2(pipefds[i * 2 + 1], STDOUT_FILENO) == -1) 
                {
                    perror("Error redirecting output");
                    exit(EXIT_FAILURE);
                }
            }

            for (int j = 0; j < 2 * numPipes; j++) {
                close(pipefds[j]);
            }

            handleDirections(currentCmd);
            if (execvp(currentCmd->arguments[0], currentCmd->arguments) == -1) {
                perror("Error executing command");
                _exit(EXIT_FAILURE);
            }
        }

        currentCmd = currentCmd->next;
        i++;
    }

    for (int j = 0; j < 2 * numPipes; j++) {
        close(pipefds[j]);
    }

    for (int j = 0; j <= numPipes; j++) {
        wait(NULL);
    }
}


void execute(cmdLine *pCmdLine)
{
    if(pCmdLine->next != NULL)
    {
        executePipe(pCmdLine);
        return;
    }
    pid_t pid = fork();
    if(pid < 0)
    {
        perror("Error forking process");
        exit(EXIT_FAILURE);
    }
    else if(pid == 0)
    {
        fprintf(stderr, "PID: %d, Executing command: %s\n", pid, pCmdLine->arguments[0]);
        handleDirections(pCmdLine);
        if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1)
        {
            perror("Error executing command");
            _exit(EXIT_FAILURE);
        }
    }
    else
    {
        //fprintf(stderr, "blocking: %d\n", pCmdLine->blocking);
        addProcess(&process_list, pCmdLine, pid);
        //printf("Adding process with command: %s\n", pCmdLine->arguments[0]);

        //printf("adding proccess PID: %d\n", pid);
        if(pCmdLine->blocking)
        {
            int status;
            waitpid(pid, &status, 0);
        }
    }
    //freeCmdLines(pCmdLine);
    /*if(execv(pCmdLine->arguments[0], pCmdLine->arguments) == -1)
    {
        perror("Error executing command");
        exit(EXIT_FAILURE);
    }*/
    
}

int main(int argc, char *argv[])
{
    char buf[PATH_MAX];
    cmdLine *line;
    history *history_list = NULL;
    int i;
    for (i = 0; i < argc; i++)
    {
        if(strcmp(argv[i], "-d") == 0)
        {
            debug = 1;
        }
    }
    while(1)
    {

        if(getcwd(buf, PATH_MAX) == NULL)
        {
            perror("Error getting current working directory");
            exit(EXIT_FAILURE);
        }
        printf("%s: ", buf);
        fflush(stdout);
        if(fgets(buf, 2048, stdin) == NULL)
        {
            fprintf(stderr, "Error reading input\n");
            exit(EXIT_FAILURE);
        }
        line = parseCmdLines(buf);
        if (line == NULL)
        {
            fprintf(stderr, "Error parsing command\n");
        }
        else if (strcmp(line->arguments[0], "quit") == 0)
        {
            freeCmdLines(line);
            freeProcessList(process_list);
            exit(EXIT_SUCCESS);
        }
        else if(strcmp(line->arguments[0], "cd") == 0)
        {
            if(chdir(line->arguments[1]) == -1)
            {
                perror("Error changing directory");
            }
        }
        else if(strcmp(line->arguments[0], "stop") == 0 ||
            strcmp(line->arguments[0], "wake") == 0 ||
            strcmp(line->arguments[0], "term") == 0) 
        {
            handleSignals(process_list, line->arguments[0], atoi(line->arguments[1]));
        }
        else if(strcmp(line->arguments[0], "procs") == 0)
        {
            printProcessListUpdated(&process_list);
        }
        else if(strcmp(line->arguments[0], "history") == 0)
        {
            printHistory(history_list);
        }
        else if(strncmp(line->arguments[0], "!", 1) == 0)
        {
            executeHistory(history_list, atoi(line->arguments[0] + 1), buf);
            line = parseCmdLines(buf);
            execute(line);
        }
        else
        {
            addToHistory(&history_list, buf);   
            if(debug)
                fprintf(stderr, "Executing command: %s\n", line->arguments[0]);
            execute(line);
        }
        freeCmdLines(line);
    }
    return 0;
}