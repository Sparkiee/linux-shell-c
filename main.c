#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


#define MAX_CMD 510
#define MAX_ARGUMENTS 10
#define MAX_ENTER_PRESSES 3

void promptPrint(int cmds, int args)
{
    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    printf("#cmd:%d|#args:%d@%s ",cmds, args, cwd);
}

int main()
{
    int total_valid_cmds = 0;
    int total_valid_args = 0;
    while(1) {
        promptPrint(total_valid_cmds, total_valid_args);

        // adding extra 2 chars for '\n' and '\0'
        char input[MAX_CMD + 2] = {0};
        int enter_count = 0;
		memset(input, 1, MAX_CMD +2);
        //receiving input from user
        while (enter_count < MAX_ENTER_PRESSES) {
            fgets(input, MAX_CMD + 2, stdin);
            if (input[0] == '\n') {
                promptPrint(total_valid_cmds,total_valid_args);
                enter_count++;
            }
            else
                enter_count = MAX_ENTER_PRESSES + 1;
            if (enter_count == 3) {
                printf("You've pressed enter 3 times, exiting the terminal...");
                exit(0);
            }
        }
        // if we've exceeded the MAX_CMD char limit;
        if (strchr(input, '\n') == NULL && !feof(stdin)) {
            promptPrint(total_valid_cmds, total_valid_args);
            printf("You've exceeded the maximum limit of %d", MAX_CMD);
            continue;
        }


        /*char * token = strtok(input, ";");
        // loop through the string to extract all other tokens
        while( token != NULL ) {
            printf(" %s\n", token); //printing each token
            char* sub_cmd = strdup(token);
            char * subtoken = strtok(sub_cmd, " ");
            int count = 0;
            char *arr[MAX_ARGUMENTS];
            while(subtoken != NULL)
            {
                arr[count++] = subtoken;
                subtoken = strtok(NULL, " ");
            }
            printf("%d", count);
            token = strtok(NULL, ";");
        }*/

        char *outer_delim = ";";
        char *inner_delim = " ";
        char *outer_token, *outer_saveptr, *inner_token, *inner_saveptr;

        outer_token = strtok_r(input, outer_delim, &outer_saveptr);
        // outter loop is splitting them by commands
        while (outer_token != NULL) {
            int count = 0;
            char *arr[MAX_ARGUMENTS];
            inner_token = strtok_r(outer_token, inner_delim, &inner_saveptr);
            // inner loop is splitting them by arguments
            while (inner_token != NULL) {
                if (inner_token == strstr)
                    printf("ye");
                arr[count++] = inner_token;
                inner_token = strtok_r(NULL, inner_delim, &inner_saveptr);
            }
            printf("count args: %d ", count);
            for (int i = 0; i < count; ++i) {
                printf("%s ", arr[i]);
            }
            //printf("\n");

            pid_t pid = fork();
            if (pid < 0) {
                perror("Fork failed");
                return 1;
            }
            if (pid == 0) // child process, run execution here
            {

            }
            outer_token = strtok_r(NULL, outer_delim, &outer_saveptr);
        }

        /*

        }*/

        // releasing all allocated memory this run
        /*for (int i = 0; i < commands_count; i++) {
            free(tokens[i]);
        }*/
    }


    return 0;
}
