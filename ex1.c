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

// a function which frees memory allocated
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

// replaces all appearances of a $key with it's value
char* replace_all_variables(const char *str, KeyVariable *vars, int variables_count) {
    char buffer[MAX_CMD * 2] = "";
    const char *start = str;
    const char *end = NULL;

    while ((end = strchr(start, '$')) != NULL) {
        strncat(buffer, start, end - start);

        char key[MAX_CMD];
        int key_len = 0;
        const char *key_start = end + 1;
        while (isalnum(*key_start) || *key_start == '_') {
            key[key_len++] = *key_start++;
        }
        key[key_len] = '\0';

        char *value = replace_value(vars, key, variables_count);
        if (value) {
            strcat(buffer, value);
            free(value);
        } else {
            // If the key is not found, keep the original text (including the $ symbol) in the output string
            strncat(buffer, end, (key_start - end));
        }

        start = key_start;
    }

    strcat(buffer, start);
    return strdup(buffer);
}

// checks whether or not, the string (or token in this case) is between quotation marks
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

int main()
{
    int total_valid_cmds = 0;
    int total_valid_args = 0;

    int variables_capacity = 10;
    int variables_count = 0;

    char cwd[256];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        printf("Error getting path");
        exit(1);
    }

    KeyVariable *vars = malloc(variables_capacity * sizeof (KeyVariable));

    while(1) {

        // adding extra 2 chars for '\n' and '\0'
        char input[MAX_CMD + 2];
        char *arguments[MAX_ARGUMENTS + 1];
        int enter_count = 0;

        //receiving input from user
        while (enter_count < MAX_ENTER_PRESSES) {
            printf("#cmd:%d|#args:%d@%s> ",total_valid_cmds, total_valid_args, cwd);
            fgets(input, MAX_CMD, stdin);
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
            printf("You've exceeded the maximum limit of %d\n", MAX_CMD);
            // Discard the remaining characters in the input buffer
            scanf("%*[^\n]%*c");
            continue;
        }

        // Remove the newline character from the input
        input[strcspn(input, "\n")] = '\0';

        char *outer_delim = ";";
        char *outer_token, *outer_saveptr;

        int in_quotes = 0;
        char *command_start = input;
        char *input_ptr = input;

        while (*input_ptr != '\0') {
            if (*input_ptr == '"') {
                in_quotes = !in_quotes;
            }

            if ((*input_ptr == ';' && !in_quotes) || *(input_ptr + 1) == '\0') {
                if (*(input_ptr + 1) == '\0') {
                    input_ptr++;
                } else {
                    *input_ptr = '\0';
                }

                char *replaced_token = replace_all_variables(command_start, vars, variables_count);
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
                bool added_var = false;
                // we are trying to split the command into X = Y, if it succeeded, we will enter the if block
                if (!is_enclosed_in_quotes(outer_token) && sscanf(outer_token, "%[^=]=%[^\n]", key, value) == 2) {
                    added_var = true;
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
                }
                if(!added_var)
                {
                    inner_token = strtok_r(outer_token, inner_delim, &inner_saveptr);
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
                            if (!end_quote_found) {
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
                                total_valid_cmds -= 1;
                                total_valid_args -= count;

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

                command_start = input_ptr + 1;
            }
            input_ptr++;
        }

    }
}