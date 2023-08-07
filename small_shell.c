#define _POSIX_C_SOURCE 200809L
#define MAX_WORDS 512
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>
#include <inttypes.h>

struct command
{
    char *parsed_array[MAX_WORDS + 1];
    char *inFile;
    char *outFile;
    int runBack;
    int array_length;
    int inFile_check;
    int outFile_check;
};

// global variables
int fore_ground_exit = 0;
int background_process = -1;
char *shell_var = "";

// function prototypes //
char *str_gsub(char *restrict *restrict, char const *restrict, char const *restrict);
int tokenString(char *, char **);
int parse_string(char **, struct command *, int);
static int change_directory(struct command *);
int exit_command(struct command *);
void build_command(struct command *, char **words, struct sigaction *);
void redirect_input_output(struct command *);
void printwords(char **, int);
void sigint_handler(int);

void printwords(char **words, int word_count)
{
    printf("\n---------WORDS ARRAY-----------\n");
    for (int i = 0; i < word_count; i++)
    {
        printf("%s, ", words[i]);
    }
    printf("\n-------------------------------\n");
}

void redirect_input_output(struct command *parsed_ptr)
// Code to handle input and output redirection
{
    if (parsed_ptr->inFile_check == 1)
    {
        int input_received = open(parsed_ptr->inFile, O_RDONLY);
        if (input_received != -1)
        {
            if (dup2(input_received, STDIN_FILENO) == -1)
            {
                fprintf(stderr, "Cannot duplicate %s and open for reading on stdin.\n", parsed_ptr->inFile);
                exit(1);
            }
        }
        if (input_received == -1)
        {
            fprintf(stderr, "Cannot open %s to read the input. \n", parsed_ptr->inFile);
            exit(1);
        }
    }
    if (parsed_ptr->outFile_check == 1)
    {
        int output_received = open(parsed_ptr->outFile, O_CREAT | O_WRONLY | O_TRUNC, 0777);
        if (output_received == -1)
        {
            fprintf(stderr, "Cannot open %s to write the output\n", parsed_ptr->outFile);
            exit(1);
        }

        if (dup2(output_received, STDOUT_FILENO) == -1)
        {
            fprintf(stderr, "Cannot duplicate %s and open for writing on stdout.\n", parsed_ptr->outFile);
            exit(1);
        }
    }
}

void build_command(struct command *parsed_ptr, char **words, struct sigaction *sigINT)
// Code to build the command and execute it
{
    // pid_t bg_child_status;
    int childStatus_one;

    // fork a new process //
    pid_t spawnPID = fork();

    switch (spawnPID)
    {
    // error handling
    case -1:
        fprintf(stderr, "unsuccessful fork");
        exit(1);
        break;

    // child process
    case 0:
        sigaction(SIGINT, sigINT, NULL);
        // reset all signals to original disposition
        redirect_input_output(parsed_ptr);

        if (execvp(words[0], words) == -1)
        {
            fprintf(stderr, "%s: %s\n", parsed_ptr->parsed_array[0], strerror(errno));
            exit(1);
        }
        break;
    default:
        if ((parsed_ptr->runBack) != 1)
        {
            if ((spawnPID = waitpid(spawnPID, &childStatus_one, 0)) >= 0)
            {

                if (WIFEXITED(childStatus_one))
                {
                    fore_ground_exit = WEXITSTATUS(childStatus_one);
                }
                else if (WIFSIGNALED(childStatus_one))
                {
                    fore_ground_exit = WTERMSIG(childStatus_one);
                }
                else if (WIFSTOPPED(childStatus_one))
                {
                    kill(spawnPID, SIGCONT);
                    fprintf(stderr, "Child process %jd stopped. Continuing. \n", (intmax_t)spawnPID);
                }
            }
        }
        else
        {
            background_process = spawnPID;
        }
        break;
    }
}

static int change_directory(struct command *path)
// Code to change the current directory
{
    char *true_path;
    char *home_var = getenv("HOME") ? getenv("HOME") : "";
    int error_check;
    if (path->array_length > 2)
    {
        goto JUMP;
    }
    if (path->array_length == 1)
    {
        true_path = home_var;
    }
    else
    {
        true_path = path->parsed_array[1];
    }

    error_check = chdir(true_path);
    if (error_check == -1)
    {
        goto JUMP;
    }
    else
    {
        return 0;
    }

JUMP:
    fprintf(stderr, "\nchanging directory was unsuccessful, error occurred.\n");
    return 1;
}

int exit_command(struct command *path)
// Code to handle the exit command
{
    if (path->array_length == 1)
    {
        /* use value of $?*/
        fprintf(stderr, "\nexit\n");
        exit(fore_ground_exit);
    }
    if (path->array_length == 2)
    {
        if ((atoi(path->parsed_array[1])) == 0)
        {
            goto ERROR;
        }
        goto FINAL;
    }

    if ((path->array_length) > 2)
    {
        goto ERROR;
    }
ERROR:;
    fprintf(stderr, "\ncannot exit, error occurred\n");
    return 1;

FINAL:;
    fprintf(stderr, "\nexit\n");
    kill(getpgid(getpid()), SIGINT);
    exit(atoi(path->parsed_array[1]));
}

char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub)
// Code to substitute a substring in a string
{
    char *str = *haystack;
    size_t haystack_len = strlen(str);
    size_t const needle_len = strlen(needle),
                 sub_len = strlen(sub);
    for (; (str = strstr(str, needle));)
    {
        ptrdiff_t off = str - *haystack;
        if (sub_len > needle_len)
        {
            str = realloc(*haystack, sizeof **haystack * (haystack_len * sub_len - needle_len + 1));
            if (!str)
                goto exit;
            *haystack = str;
            str = *haystack + off;
        }
        memmove(str + sub_len, str + needle_len, haystack_len + 1 - off - needle_len);
        memcpy(str, sub, sub_len);
        haystack_len = haystack_len + sub_len - needle_len;
        str += sub_len;
        if (strcmp(needle, "~") == 0)
        {
            break;
        }
    }
    str = *haystack;
    if (sub_len < needle_len)
    {
        str = realloc(*haystack, sizeof **haystack * (haystack_len + 1));
        if (!str)
            goto exit;
        *haystack = str;
    }
exit:
    return str;
}

int tokenString(char *line, char **words)
// Code to tokenize the input string
{
    /* array of string pointers, for now setting to 512 pointers max */
    int i = 0;
    int k;
    char *localIFS;
    localIFS = getenv("IFS");
    if (localIFS == NULL)
    {
        localIFS = " \t\n";
    }
    /* tokenize everything and store in a token array */
    char *token = strtok(line, localIFS);
    while (token != NULL)
    {
        words[i] = strdup(token);
        token = strtok(NULL, localIFS);
        i++;
    }
    words[i] = NULL; /* was token */
    /* iterate through each token to expand and parse */
    char *home;
    home = getenv("HOME");
    if (home == NULL)
    {
        home = "";
    }
    else
    {
        home = getenv("HOME");
    }
    for (k = 0; k < i; k++)
    {
        // check to see if index[0] is ~ and index[1] is /, if so call str_gsb with ~ as needle //
        // put home env var as sub, and don't recursively expand //
        if (strncmp(words[k], "~/", 2) == 0)
        {
            words[k] = str_gsub(&words[k], "~", home);
        }
        int pid = getpid();
        char *mypid = malloc(10);
        sprintf(mypid, "%d", pid);
        char *fg_string = malloc(10);
        sprintf(fg_string, "%d", fore_ground_exit);
        if ((strstr(words[k], "$$")) != NULL)
        {
            words[k] = str_gsub(&words[k], "$$", mypid);
        }
        if ((strstr(words[k], "$?")) != NULL)
        {
            words[k] = str_gsub(&words[k], "$?", fg_string);
        }
        if ((strstr(words[k], "$!")) != NULL)
        {
            char *bg_process = malloc(10);
            if (background_process == -1)
            {
                bg_process = "";
                words[k] = str_gsub(&words[k], "$!", bg_process);
            }
            else
            {
                sprintf(bg_process, "%d", background_process);
                words[k] = str_gsub(&words[k], "$!", bg_process);
            }
        }
    }
    return i;
}

int parse_string(char **words, struct command *parsed_ptr, int token_array_length_temp)
// Code to parse the token array and handle input/output redirection
{
    int j;
    int x = 0;

    for (j = 0; j < token_array_length_temp; j++)
    {
        if (strcmp(words[j], ">") == 0)
        {
            words[j] = NULL;
            parsed_ptr->outFile = words[j + 1];
            parsed_ptr->outFile_check = 1;
            j++;
        }
        else if (strcmp(words[j], "<") == 0)
        {
            words[j] = NULL;
            parsed_ptr->inFile = words[j + 1];
            parsed_ptr->inFile_check = 1;
            j++;
        }
        else if (strcmp(words[j], "&") == 0)
        {
            words[j] = NULL;
            parsed_ptr->runBack = 1;
        }
        else if (strcmp(words[j], "#") == 0)
        {
            words[j] = NULL;
        }
    }
    while (words[x] != NULL)
    {
        parsed_ptr->parsed_array[x] = words[x];
        x++;
    }
    return 0;
}

void sigint_handling(int signo)
// Signal handler for SIGINT (Ctrl+C)
{
}

int main()
// Main function where the shell runs
{
    // SIGSTOP FOR CTRL+Z
    struct sigaction sigSTOP_action = {0};
    sigSTOP_action.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &sigSTOP_action, NULL);
    // SIGINT CTRL+C
    struct sigaction sigINT_action = {0};
    sigINT_action.sa_handler = sigint_handling;
    /* stores the line, after expansion and parsing in a command class */
    struct command parsed_info;
    struct command *parsed_ptr = &parsed_info;
    /* set $? and $! */
    int token_array_length_temp;
    /* store the line */
    char *line = NULL;
    /* store the word(s) in the token array of token pointers */
    char *words[512];
    char *myPS1 = getenv("PS1");
    size_t word_len = 0;
    /* set errno to 0 before the loop? */
    errno = 0;

    while (1)
    {

        pid_t bg_child_status;
        int childStatus;
        while ((bg_child_status = waitpid(0, &childStatus, WNOHANG | WUNTRACED)) > 0)
        {
            if (WIFEXITED(childStatus))
            {
                int exit_status = WEXITSTATUS(childStatus);
                fprintf(stderr, "Child process %jd done. Exit status %d.\n", (intmax_t)bg_child_status, exit_status);
            }
            else if (WIFSIGNALED(childStatus))
            {
                int sig_status = WTERMSIG(childStatus);
                fprintf(stderr, "Child process %jd done. Signaled %d.\n", (intmax_t)bg_child_status, sig_status);
            }
            else if (WIFSTOPPED(childStatus))
            {
                fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t)bg_child_status);
                kill(bg_child_status, SIGCONT);
            }
        }
        if (myPS1 == NULL)
        {
            myPS1 = "";
        }
        /* print prompt */
        fprintf(stderr, "%s", myPS1);
        // set SIGINT to empty
        sigaction(SIGINT, &sigINT_action, NULL);
        getline(&line, &word_len, stdin);
        // set SIGINT back to ignore
        sigaction(SIGINT, &sigSTOP_action, NULL);
        /* handle EOF */
        if (feof(stdin) != 0)
        {
            fprintf(stderr, "\nexit\n");
            /* set the $? to a non-zero value, go back to the beginning of the loop */
            exit(fore_ground_exit);
        }
        /* signal handling, print a new line, re-loop */
        /* split input into words */
        token_array_length_temp = tokenString(line, words);
        /* parse the token array */
        parse_string(words, parsed_ptr, token_array_length_temp);
        (parsed_ptr->array_length) = token_array_length_temp;
        char *first_arg = parsed_ptr->parsed_array[0];
        if (!first_arg || (strcmp(first_arg, "#") == 0))
        {
            goto CLEANUP;
        }
        /* do the exit command */
        else if (strcmp(first_arg, "exit") == 0)
        {
            exit_command(parsed_ptr);
        }
        /* do change directory */
        else if (strcmp(first_arg, "cd") == 0)
        {
            change_directory(parsed_ptr);
            goto CLEANUP;
        }
        /* execute non-built-in command */
        else
        {
            build_command(parsed_ptr, words, &sigINT_action);
            goto CLEANUP;
        }
        goto CLEANUP;

    CLEANUP:;
        int m;
        int n;
        parsed_ptr->inFile = NULL;
        parsed_ptr->outFile = NULL;
        parsed_ptr->array_length = -1;
        parsed_ptr->inFile_check = 0;
        parsed_ptr->outFile_check = 0;
        parsed_ptr->runBack = 0;
        for (m = 0; m < 513; m++)
        {
            words[m] = NULL;
        }
        for (n = 0; n < 513; n++)
        {
            parsed_ptr->parsed_array[n] = NULL;
        }
    }
}