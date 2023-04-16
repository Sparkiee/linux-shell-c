#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define bool int
#define true 1
#define false 0

#define MAX_CMD 510
#define MAX_ARGUMENTS 10
#define MAX_ENTER_PRESSES 3

// using this struct to store variables
typedef struct {
    char *key;
    char *value;
} KeyVariable;

void free_key_variables(KeyVariable *vars, int count) {
    for (int i = 0; i < count; i++) {
        free(vars[i].key);
        free(vars[i].value);
    }
}

// finds whether a key exists or not
int find_key(KeyVariable *table, const char *key, int count)
{
    for (int i = 0; i < count; i++) {
        if (strcmp(table[i].key, key) == 0)
            return i;
    }
    return -1;
}

// returns a value for a key
char* replace_value(KeyVariable *table, const char *key, int count)
{
    for (int i = 0; i < count; i++) {
        if (strcmp(table[i].key, key) == 0) {
            return strdup(table[i].value);
        }
    }
    return NULL;
}


char* replace_all_variables(const char *str, KeyVariable *vars, int variables_count) {
    char buffer[MAX_CMD * 2] = "";
    const char *start = str;
    const char *end = NULL;
    int in_quotes = 0;

    while (*start != '\0') {
        if (*start == '"') {
            in_quotes = !in_quotes;
        }

        if (*start == '$' && !in_quotes) {
            strncat(buffer, str, start - str);

            char key[MAX_CMD];
            int key_len = 0;
            const char *key_start = start + 1;
            while (isalnum(*key_start) || *key_start == '_') {
                key[key_len++] = *key_start++;
            }
            key[key_len] = '\0';

            char *value = replace_value(vars, key, variables_count);
            if (value) {
                if (value[0] != '"' || value[strlen(value) - 1] != '"') {
                    char *quoted_value = (char *)malloc(strlen(value) + 3);
                    if (quoted_value == NULL) {
                        perror("ERR\n");
                        exit(1);
                    }
                    sprintf(quoted_value, "\"%s\"", value);
                    strcat(buffer, quoted_value);
                    free(quoted_value);
                } else {
                    strcat(buffer, value);
                }
                free(value);
            }

            str = key_start;
            start = str;
        } else {
            start++;
        }
    }

    strcat(buffer, str);
    return strdup(buffer);
}

bool is_enclosed_in_quotes(const char *str) {
    const char *first_quote = strchr(str, '"');
    const char *last_quote = strrchr(str, '"');
    const char *equal_sign = strchr(str, '=');

    return first_quote && last_quote && equal_sign &&
           first_quote < equal_sign && equal_sign < last_quote;
}

// a function to easily remove spaces in a string
void remove_spaces(char *str) {
    char *dst = str;

    while (*str != '\0') {
        if (!isspace(*str)) {
            *dst = *str;
            dst++;
        }
        str++;
    }

    *dst = '\0';
}

// prints the linux prompt format
void promptPrint(int cmds, int args)
{
    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    if (cwd == NULL)
        printf("error getting PWD!!!\n");
    printf("#cmd:%d|#args:%d@%s> ",cmds, args, cwd);
}

int main()
{
    int total_valid_cmds = 0;
    int total_valid_args = 0;

    int variables_capacity = 10;
    int variables_count = 0;

    KeyVariable *vars = malloc(variables_capacity * sizeof (KeyVariable));
    while(1) {

        // adding extra 2 chars for '\n' and '\0'
        char input[MAX_CMD + 2];
        char *arguments[MAX_ARGUMENTS + 1];
        int enter_count = 0;

        //receiving input from user
        while (enter_count < MAX_ENTER_PRESSES) {

            promptPrint(total_valid_cmds, total_valid_args);
            fgets(input, MAX_CMD + 2, stdin);
            if (input[0] == '\n') {
                enter_count++;
            }
            else
                enter_count = MAX_ENTER_PRESSES + 1;
            if (enter_count == 3) {
                printf("You've pressed enter 3 times, exiting the terminal...");

                // free memory at the end of the program
                free_key_variables(vars, variables_count);
                free(vars);
                exit(0);
            }
        }

        // if we've exceeded the MAX_CMD char limit;
        if (strchr(input, '\n') == NULL && !feof(stdin)) {
            printf("You've exceeded the maximum limit of %d", MAX_CMD);
            continue;
        }

        // Remove the newline character from the input
        input[strcspn(input, "\n")] = '\0';

        char *outer_delim = ";";
        char *outer_token, *outer_saveptr;

        outer_token = strtok_r(input, outer_delim, &outer_saveptr);
        // outer loop is splitting them by commands
        while (outer_token != NULL) {
            char *replaced_token = replace_all_variables(outer_token, vars, variables_count);
            outer_token = replaced_token;

            int count = 0;
            // resetting the array of arguments in case of junk
            for (int i = 0; i < MAX_ARGUMENTS; ++i) {
                arguments[i] = NULL;
            }
            char *inner_delim = " ";
            char *inner_token, *inner_saveptr;

            //before we split the command into arguments, we will check if we're trying to save a var
            char key[MAX_CMD], value[MAX_CMD];
            // we are trying to split the command into X = Y, if it succeeded, we will enter the if block
            if (!is_enclosed_in_quotes(outer_token) && sscanf(outer_token, "%[^=]=%[^\n]", key, value) == 2) {
                remove_spaces(key);
                if(variables_count >= variables_capacity)
                {
                    variables_capacity *= 2;
                    vars = realloc(vars, variables_capacity * sizeof(KeyVariable));
                }
                int index = find_key(vars, key, variables_count);
                if(index == -1)
                {
                    vars[variables_count].key = strdup(key);
                    vars[variables_count].value = strdup(value);
                    variables_count++;
                } else {
                    free(vars[index].value);
                    vars[index].value = strdup(value);
                }

                // we found a variable, so we are done with current command, skipping to the next
                // and skipping current itteration
                inner_token = NULL;
                free(replaced_token);
                outer_token = strtok_r(NULL, outer_delim, &outer_saveptr);
                continue;
            }

            inner_token = strtok_r(outer_token, inner_delim, &inner_saveptr);
            // inner loop is splitting them by arguments

            while (inner_token != NULL) {
                // we are checking for a beginning of a quote
                if(inner_token[0] == '"') {
                    bool end_quote_found = false;
                    size_t len = strlen(inner_token);
                    char* quoted_token = malloc(len + 1);
                    strcpy(quoted_token, inner_token + 1);
                    char *loc = strchr(quoted_token, '"');
                    if(loc != NULL) {
                        end_quote_found = true;
                        *loc = '\0';
                    }
                    // if we found a beginning of a quote, we need to start looping for the end of it,
                    while (!end_quote_found) {
                        inner_token = strtok_r(NULL, "\"", &inner_saveptr);
                        if (inner_token == NULL) {
                            end_quote_found = true;
                            continue;
                        }
                        len = strlen(inner_token);
                        if(inner_token[len - 1] == '"') {
                            end_quote_found = true;
                            inner_token[len - 1] = '\0'; // Remove the closing quote
                        }
                        quoted_token = realloc(quoted_token, strlen(quoted_token) + len + 2);
                        strcat(quoted_token, " ");
                        strcat(quoted_token, inner_token);
                    }
                    inner_token = quoted_token;
                }

                // adding the argument into the arguments
                arguments[count++] = inner_token;
                inner_token = strtok_r(NULL, inner_delim, &inner_saveptr);
            }

            /*printf("\n");
            for (int i = 0; i < count; ++i) {
                printf("%s\n", arguments[i]);
            }*/

            bool command_err = false;
            if(count == 11)
            {
                printf("You've exceeded the number of allowed arguments\n");
                command_err = true;
            }
            if(strcmp(arguments[0], "cd") == 0)
            {
                printf("cd not supported\n");
                command_err = true;
            }

            if(command_err)
            {
                inner_token = NULL;
                outer_token = strtok_r(NULL, outer_delim, &outer_saveptr);
            }
            else
            {
                total_valid_cmds += 1;
                total_valid_args += count;

                pid_t pid = fork();
                if (pid < 0) // fork failed
                {
                    fprintf(stderr, "ERR\n");
                    exit(1);
                }
                else if(pid == 0) // child process
                {

                    if(execvp(arguments[0], arguments) == -1) // checking if execution failed of cmd
                    {
                        printf("ERR\n");
                    }
                }
                else // parent process
                {
                    int status;
                    waitpid(pid, &status, 0);
                    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                        //printf("Child process exited with an error, terminating the parent process...\n");
                        // child process has quit with an error, therefore exitting the programl
                        free_key_variables(vars, variables_count);
                        free(vars);
                        exit(1);
                    }
                }
                outer_token = strtok_r(NULL, outer_delim, &outer_saveptr);
            }

        }
    }
}