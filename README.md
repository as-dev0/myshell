# myshell

This is a shell program for Linux in which you can run commands. Input and output redirection are supported using the '<' and '>' symbols. Background processes are supported using the '&' symbol. In any command containing the two consecutive characters '$$', the '$$' will be replaced by the pid of the process running the command.

To compile:  
gcc -std=c99 myshell.c -o myshell

To execute:  
./myshell

How to Use:  
Run commands as you normally would in a terminal. You may exit the program by entering the command 'exit'.

Example:  

![C](https://user-images.githubusercontent.com/114560165/199879799-219ce056-d6aa-4296-939d-a63f31ed079e.png)


Known Issues:  
If your PATH environment variable contains directories which don't exist, the program might crash.
