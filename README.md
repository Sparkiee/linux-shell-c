# Basic Linux Shell in C
Authored by Maxim Shteingard
209171156

== Description == 
	The program is a simulation of a Linux shell with command execution and support for variables. The shell allows users to enter and execute commands, separated by semicolons, with arguments. Additionally, the shell supports defining and using variables with the '$' prefix.

	The shell also keeps track of the total number of valid commands and arguments executed. When the user enters an invalid command, the shell informs the user about the error and continues to accept new input.

== Program Database ==:
	• KeyVariable struct for storing variables (key-value pairs, a 'HashTable-like' structure).

==Defines==:
	• '#define bool int', #define true 1, and #define false 0 are used to enable a more readable and aesthetic code style when working with boolean values, instead of using 1s and 0s directly.
	
	• MAX_CMD - to determine the max length for a command
	
	• MAX_ARGUMENTS - to determine the max amount of arguments, The last argument is reserved to ensure that we can easily check whether the limit of 10 arguments has been exceeded and to execute a command with 'X' arguments, where the 'X+1' argument must be NULL.
	
	• MAX_ENTER_PRESSES - to determine the amount of consecutive 'Enter' pressed required to exit the current program 

==Functions==:
	• find_key - the method receives the variables table, a key and count (which is the number of variables we have stored at the moment), and locates the value for the specified key and returns it's position in the database
	
	• replace_value - he method receives the variables table, a key and count (which is the number of variables we have stored at the moment), it located the value for the specified key, if it exits it will return the value, otherwise returns NULL

	• replace_all_variables - the method receives the command string, the keyVariables table and the count, it searches for all the variable appearing in the command, and replaces them with the appropriate value if it exists

	• remove_spaces - it receives a string and removes the spaces in it

	• promptPrint - receives the number of successful commands and arguments and prints it to the console in the format of #cmd:<sum of cmds>|#args:<sum of args>@<path>'


==How to compile?==:
	compile: gcc main.c -o a.out
	run: ./a.out
