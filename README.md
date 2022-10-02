# smallshell

This is a small shell program for Linux in which you can run commands. Input and output redirection are supported using the '<' and '>' symbols. Background processes are supported using the '&' symbol. In any command containing the two consecutive characters '$$', the '$$' will be replaced by the pid of the process running the command.

To compile:  
gcc -std=c99 smallsh.c -o smallsh

To execute:  
./smallsh

How to Use:  
Run commands as you normally would in a terminal. You may exit the program by entering the command 'exit'.

Known Issues:  
If your path environment variable contains directories which don't exist, the program might crash.

Example:  
![small](https://user-images.githubusercontent.com/114560165/193472627-89a35b6f-b45d-4498-8097-07a02da6affb.png)
