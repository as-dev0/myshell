# smallshell

This is a small shell program in which you can run commands. Input and output redirection are supported using the '<' and '>' symbols. Background processes are supported using the '&' symbol. In any command containing the two consecutive characters '$$', the '$$' will be replaced by the pid of the process running the command.

To compile:
gcc -std=c99 smallsh.c -o smallsh

To execute:
./smallsh
