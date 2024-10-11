#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
 
// print out the error message and exit with errno if something is wrong
// the message printer should be deleted for testing 
static void check_error(int ret, const char *message) {
    if (ret != -1) {
        return;
    }
    int err = errno;
    perror(message);
    exit(err);
}

int main(int argc, char *argv[]) {
    if (argc < 2){
        return EINVAL; 
    } 
    
    // if there is no pipe required
    if (argc == 2) {
        pid_t pid = fork();
        if (pid== 0) {
            check_error(execlp(argv[1], argv[1], NULL), "execlp");
        }
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)){
            int exit_code = WEXITSTATUS(status);
            if (exit_code != 0) {
                return exit_code;
            }
        }
        return 0;
    }
    
    // initialize all the pipes up until 10
    int pipefd[10][2];
    for (int i = 0; i < 10; i++) {
        check_error(pipe(pipefd[i]), "pipe");
    }
    int pid_array[argc - 1];
    int i = 0;
    while (i < argc - 1) {
        pid_t pid = fork();
        if (pid == 0) {
            if (i == 0) {
                // first argv
                for (int j = 0; j < 10; j++) {
                    if (j != 0) {
                        check_error(close(pipefd[j][1]), "close first1 argv");
                    }
                    check_error(close(pipefd[j][0]), "close first2 argv");
                }
                check_error(dup2(pipefd[0][1], STDOUT_FILENO), "dup2"); // replaces write end of first pipe with STD_OUT
                check_error(close(pipefd[0][1]), "close first3 argv");
                check_error(execlp(argv[1], argv[1], NULL), "execlp");
            } else if (i == (argc - 2)) {
                // last argv
                for (int j = 0; j < 10; j++) {
                    check_error(close(pipefd[j][1]), "close last1 argv");
                    if (j != i - 1) {
                        check_error(close(pipefd[j][0]), "close last2 argv");
                    }
                }
                check_error(dup2(pipefd[i - 1][0], STDIN_FILENO), "dup2"); // replaces read end of last pipe with STD_IN
                check_error(close(pipefd[i - 1][0]), "close last3 argv");
                check_error(execlp(argv[i + 1], argv[i + 1], NULL), "execlp");
            } else {
            for (int j = 0; j < 10; j++) {
                if (j != i){
                    check_error(close(pipefd[j][1]), "close1");
                }
                if (j != i - 1) {
                    check_error(close(pipefd[j][0]), "close2");
                }
            }
            check_error(dup2(pipefd[i][1], STDOUT_FILENO), "dup2"); // write end of current execution replaces the standard output
            check_error(dup2(pipefd[i - 1][0], STDIN_FILENO), "dup2"); // read end of next execution replaces the standard input
            check_error(close(pipefd[i][1]), "close3"); // close the file descriptor of write end of current execution
            check_error(close(pipefd[i - 1][0]), "close4"); // close the file descriptor of read end of last execution
            check_error(execlp(argv[i + 1], argv[i + 1], NULL), "execlp");
            }
        }
        // parent process
        pid_array[i] = pid;
        i++;
    }
    
    // close all the file descriptors in the parent process
    for (int i = 0; i < 10; i++) {
        check_error(close(pipefd[i][1]), "close");
        check_error(close(pipefd[i][0]), "close");
    }
    
    // check the children exit status
    for (int i = 0; i < argc - 1; i++){
        int status;
        waitpid(pid_array[i], &status, 0);
        if (WIFEXITED(status)){
            int exit_code = WEXITSTATUS(status);
            if (exit_code != 0) {
                return exit_code;
            }
        }
    }
    return 0;
}
