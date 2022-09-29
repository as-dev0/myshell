#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>


// Helps keep track of whether or not we are in foreground only mode
static int foregroundOnly = 2;


// Takes one argument, a string, and sets it to an empty string
void empty(char* string){
  memset(string,0,strlen(string));
}


// Takes one argument, a string, and prints it to stdout
void w(char *string){

  fflush(stdout);
  write(STDOUT_FILENO,string,strlen(string));
  fflush(stdout);

}


// Prints a new line
void n() {

  fflush(stdout);
  write(STDOUT_FILENO,"\n",1);
  fflush(stdout);

}


// Takes one argument, a number, and prints it to stdout
void wn(int number){

  char tempn[200]; empty(tempn);
  sprintf(tempn, "%i", number);
  w(tempn);

}


// Takes two arguments, 
// an integer named input_fd representing an input file descriptor ,
// a string named command, 
// and reads input from the input file descriptor into command which is expected to be empty.
// Returns the number of bytes read not including the new line character.
int getInput(int input_fd, char *command){

  write(STDOUT_FILENO,":",1);
  fflush(stdout);
  int nRead = 0;

  while (nRead <= 2048){

    // We are reading from stdin one character at a time
    // so that we may stop when encountering a new line
    char c[2]; empty(c);
    read(input_fd, c, 1);

    // Prevents infinite loops if the user enters ^Z
    if(strlen(c) == 0){
      break;
    }

    strcat(command,c);

    // Stop reading when we encounter a new line
    // nRead will be the number of non-new line characters entered due to the break
    if (command[nRead] == '\n'){
      break;
    }

    nRead++;

  }
  
  return nRead;

}


// Takes one argument, a string named string.
// Returns 1 if there is no occurrence of the $$ symbol in the string
// Returns 0 otherwise
int isExpanded(char *string){

  for (size_t i = 0; i < strlen(string)-1; i++){
    if ((string[i] == '$') && (string[i+1] == '$')){
      return 0;
    }
  }

  return 1;

}


// Takes three arguments, 
// a string named unexpanded
// a string named expanded which is expected to be empty
// a number named pid,
// and replaces the first occurrence of $$ in unexpanded with pid
// and stores the result in expanded
void expand(char *unexpanded, char *expanded, int pid){
  
  for (size_t i = 0; i < strlen(unexpanded)-1; i++){

    // Checks for occurrence of $$
    if ((unexpanded[i] == '$') && (unexpanded[i+1] == '$')){
      
      // Loop runs up to but not including the first $ character
      for (size_t j = 0; j < i; j++){

        // This block appends unexpanded[i] to expanded
        char c[2] = ""; empty(c);
        strncpy(c,(char *)unexpanded + j,1); // Copies the character of unexpanded at position j into c
        strcat(expanded,c); // Appends the character c to expanded
      }

      // This block appends the pid to expanded
      char cpid[20]; // cpid is a string that will contain pid
      sprintf(cpid,"%i",pid);
      strcat(expanded,cpid);

      // Loops runs from the character after the second $ to the end of unexpanded
      for (size_t j = i+2; j < strlen(unexpanded); j++){

        // This block appends unexpanded[i] to expanded
        char c[2] = ""; empty(c);
        strncpy(c,(char *)unexpanded + j,1); // Copies the character of unexpanded at position j into c
        strcat(expanded,c); // Appends the character c to expanded
      }

      // At this point, we have looped through the whole string so we break
      break;
    }
  }
}


// Takes three arguments, 
// a string named unexpanded
// a string named expanded which is expected to be empty
// a number named pid,
// and replaces all occurrences of $$ in unexpanded with pid
// and stores the result in expanded
void fullExpand(char *unexpanded, char *expanded, int pid){

  strcpy(expanded,unexpanded);
  // cunex will be a copy of unexpanded at every iteration
  // of the following while loop
  char cunex[2049] = ""; empty(cunex);

  while(isExpanded(expanded) == 0){

    expand(expanded,cunex,pid);
    strcpy(expanded,cunex);
    empty(cunex);

  }
}


// Takes three arguments, 
// a integer named argValue
// a string named command
// a string named arg which is expected to be empty,
// and copies the argument of command at position argValue into arg
// The first argument of a command has argValue 1
void getArgument(int argValue, char *command, char* arg){

  int numberOfSpacesPassed = 0;

  for (size_t i = 0; i < strlen(command); i++){

    if (command[i] == ' '){
      numberOfSpacesPassed += 1;
      continue;
    }

    if (argValue == numberOfSpacesPassed){
      break;
    }

    // At this point, i will loop through the argument
    // we are interested in
    if (argValue == numberOfSpacesPassed+1){

      // Appends command[i] to arg
      char c[2] = ""; empty(c);
      strncpy(c,(char *)command + i,1);
      strcat(arg,c);

    }
  }
}


// Takes three arguments, 
// a integer named dirNumber
// a string named path
// a string named result which is expected to be empty,
// and copies the direction of path at position dirNumber into result
// The first directory of a path has dirNumber 1
// For example, if dirNumber=1, path="/bin:/usr/bin", result=""
// after running the function, result="/bin"
// For example, if dirNumber=2, path="/bin:/usr/bin", result=""
// after running the function, result="/usr/bin"
void getPathDirectory(int dirNumber, char *path, char* result){

  int numberOfSpacesPassed = 0;

  for (size_t i = 0; i < strlen(path); i++){

    if (path[i] == ':'){
      numberOfSpacesPassed += 1;
      continue;
    }

    if (dirNumber == numberOfSpacesPassed){
      break;
    }

    // At this point, i will loop through the directory we are interested in
    if (dirNumber == numberOfSpacesPassed+1){

      // Appends the ith character of path to result
      char c[2] = "";
      strncpy(c,(char *)path + i,1);
      strcat(result,c);

    }
  }
}




// Takes one argument, a string names pathvariable, and returns
// the number of directories in the pathvariable
// For example, if pathvariable="/bin", the function will return 1
// For example, if pathvariable="/bin:/usr/bin", the function will return 2
int getNumberPathDirectories(char *pathvariable){

  int counter = 0;

  for (size_t i = 0; i < strlen(pathvariable); i++){

    if (pathvariable[i] == ':'){
      counter += 1;
    }
  }

  return counter+1;
}


// Takes one argument, a string names command, and returns
// the number of arguments in the command
// For example, if command="sleep 5", the function will return 2
// For example, if command="ls", the function will return 1
int getNumberCommandArguments(char *command){

  int counter = 0;

  for (size_t i = 0; i < strlen(command); i++){

    if (command[i] == ' '){
      counter += 1;
    }
  }

  return counter+1;
}


// Takes two arguments, 
// a string named arg, 
// a string named command,
// and returns the position of arg in command. If arg is not found in command, returns -1
// For example, if arg="sleep" and command="sleep 5", the function will return 1
// For example, if arg="5" and command="sleep 5", the function will return 2
int getArgumentPosition(char *arg, char *command){

  for (int i = 0; i < getNumberCommandArguments(command); i++){
    
    char tempArg[2049]; empty(tempArg);
    getArgument(i,command,tempArg); // After this, tempArg will contain the ith argument of command

    if (strcmp(tempArg,arg) == 0){
      return i;
    }

  }

  return -1;

}


// Takes two arguments, 
// a string named command, 
// a string named path,
// and returns the position of command in path. If command is not found in path, returns -1
// For example, if command="sleep" and path="/bin:/usr/bin", the function will return 1 because sleep is found in /bin
int searchCommand(char *command, char* path){

  int numberD = getNumberPathDirectories(path);
  // Will contain a directory in the path environment variable
  char d[2048] = ""; empty(d);

  for (int i = 1; i < numberD - 1; i++){

    empty(d);
    getPathDirectory(i, path, d);

    DIR* dir = opendir(d);
    struct dirent *entry;

    // Searches for the command in the ith directory of the path environment variable
    // If command is found in the directory, i is returned
    while((entry = readdir(dir)) != NULL){
        if ((strcmp(entry->d_name, command) == 0)){
            closedir(dir);
            return i;
            
        }
    }
    closedir(dir);
  }
  return -1;

}


// Takes two arguments, 
// a string named command, 
// a string named result,
// If command is found in a directory of the path environment variable, result will contain the path of the command in that directory
// If command is not found in a directory of the path environment variable, the function does nothing
// For example, if command="sleep" and path="/bin:/usr/bin", after running the function result="/bin/sleep" because sleep is in bin/
void getCommandWithPath(char* command, char* result){

  int isFound = searchCommand(command, getenv("PATH"));

  if (isFound != -1) {

    char path[500]; empty(path);
    getPathDirectory(isFound, getenv("PATH"), path);
    
    strcat(result, path);
    strcat(result, "/");
    strcat(result, command);

  }

}



// Handler for SIGTSTP when not in foregroundOnly mode
void handle_SIGTSTP1(int signo){

  write(STDOUT_FILENO, "\n", 1);
  fflush(stdout);
  char message[] = "Entering foreground-only mode (& is now ignored)";
  write(STDOUT_FILENO, message, 48);
  fflush(stdout);
  write(STDOUT_FILENO, "\n", 1);
  fflush(stdout);

  // This if statement does nothing and is included to compile without the unused parameter warning
  // when compiling with -Wall -Wextra -Wpedantic -Werror
  if (signo == 100){
    ;
  }
}


// Handler for SIGTSTP when in foregroundOnly mode
void handle_SIGTSTP2(int signo){

  write(STDOUT_FILENO, "\n", 1);
  fflush(stdout);
  char message[] = "Exiting foreground-only mode";
  write(STDOUT_FILENO, message, 28);
  fflush(stdout);
  write(STDOUT_FILENO, "\n", 1);
  fflush(stdout);

  // This if statement does nothing and is included to compile without the unused parameter warning
  // when compiling with -Wall -Wextra -Wpedantic -Werror
  if (signo == 100){
    ;
  }
}


// Master handler for SIGTSTP
// Decides which of handle_SIGTSTP1 or handle_SIGTSTP2 to run
void handle_SIGTSTP(int signo){

  // The static variable foregroundOnly defined at the beginning of
  // the file is used to keep track of the modes
  // If foregroundOnly is even, foregroundOnly mode is turned on
  // If foregroundOnly is odd, foregroundOnly mode is turned off

  if (foregroundOnly %2 == 0){
    foregroundOnly ++;
    handle_SIGTSTP1(signo);
  } else {
    foregroundOnly ++;
    handle_SIGTSTP2(signo);
  }

}


// Takes two arguments,
// an array of pointers named newarg which is expected to be empty
// a string named expandedCommand,
// and fills the array newarg with the arguments of expandedCommand
// Returns -1 if command not found, and 0 otherwise
// For example, if expandedCommand="sleep 5", newarg = {"/bin/sleep","5",NULL}
int fillUpArgArray(char **newarg, char* expandedCommand){

  // argC will contain the first argument of expandedCommand
  char argC[2049] = ""; empty(argC);
  getArgument(1,expandedCommand,argC);

  // arg1 will contain the concatentation of a directory and the first argument of expandedCommand
  char arg1[2049] = ""; empty(arg1);
  getCommandWithPath(argC, arg1);

  if (strlen(arg1)==0){
    return -1;
  }

  // Copies arg1 into first element of newarg
  newarg[0] = malloc(strlen(arg1));
  strcpy(newarg[0],arg1);

  // This loop appends the remaining arguments of expandedCommand to newarg
  for (int nArg = 2; nArg < getNumberCommandArguments(expandedCommand)+1; nArg++){

    char tempArg[2049] = ""; empty(tempArg);
    getArgument(nArg,expandedCommand,tempArg);

    newarg[nArg-1] = malloc(strlen(tempArg));
    strcpy(newarg[nArg-1],tempArg);

  }

  // The array ends with a NULL
  newarg[getNumberCommandArguments(expandedCommand)] = NULL;

  return 0;

}


// Takes two arguments,
// an integer numberBackgroundPid which is the length of arrayBackgroundPid
// an array named arrayBackgroundPid containing pids of background processes,
// and displays a message for any background process whose pid is in arrayBackgroundPid 
// that has terminated showing its exit value or termination signal
// This is run before prompting the user for input.
void messageTerminatedBackgroundChild(int numberBackgroundPid, int *arrayBackgroundPid){

  for (int b = 0; b < numberBackgroundPid; b++){

    int checkStatus = 0;

    int p = waitpid(arrayBackgroundPid[b], &checkStatus, WNOHANG);

    // If this is true, then this process has exited or terminated
    if (p != 0 && p != -1){

      w("background pid ");
      wn(p);
      w(" is done: ");
      
      if (WIFEXITED(checkStatus)){
        w("exit value ");
        wn(WEXITSTATUS(checkStatus));
      }

      if (WIFSIGNALED(checkStatus)){
        w("terminated by signal ");
        wn(WTERMSIG(checkStatus));
      }

      n();

    }
  }

}


// Takes one argument, a string, and removes the ampersand symbol from it
// For example, if string="sleep 5 &", after running the function, string="sleep 5"
void removeAmpersand(char* string){

  char duplicateCommand[2100] = ""; empty(duplicateCommand);

  strncpy(duplicateCommand, string, strlen(string) - 2); // 2 bytes are removed, one for the & and one for the space preceding the &
  empty(string);
  strncpy(string, duplicateCommand, strlen(duplicateCommand));

}


// Takes one argument, a string, and returns 1 if it contains the string " < "
// Returns 0 otherwise
int isInputRedirected(char* command){

  for (size_t i = 0; i < strlen(command); i++){

    // Checks for existence of input redirector
    if (command[i] == '<'){

      // The next two lines check if the input redirector is between two spaces
      if ( i-1 > 0 && i+1 < strlen(command)){
        if (command[i-1] == ' ' && command[i+1] == ' '){

          // Checks if input redirector has at least two characters before and after it
          // The smallest possible expression containing an input redirector is: a < b
          if (i > 1 && i < strlen(command)-2){
            return 1;
          }
        }
      }
    }

  }
  return 0;

}


// Takes one argument, a string, and returns 1 if it contains the string " > "
// Returns 0 otherwise
int isOutputRedirected(char* command){

  for (size_t i = 0; i < strlen(command); i++){

    // Checks for existence of output redirector
    if (command[i] == '>'){

      // The next two lines check if the output redirector is between two spaces
      if ( i-1 > 0 && i+1 < strlen(command)){
        if (command[i-1] == ' ' && command[i+1] == ' '){

          // Checks if output redirector has at least two characters before and after it
          // The smallest possible expression containing an output redirector is: a > b
          if (i > 1 && i < strlen(command)-2){
            return 1;
          }
        }
      }
    }

  }
  return 0;

}


// Takes two arguments,
// an integer numberBackgroundPid which is the length of arrayBackgroundPid
// an array named arrayBackgroundPid containing pids of background processes,
// and terminates any background process whose pid is in arrayBackgroundPid
// that hasn't already exited or terminated
// This is run when the user enters "exit"
void terminateBackgroundProcessses(int numberBackgroundPid, int *arrayBackgroundPid){

  for (int b = 0; b < numberBackgroundPid; b++){

    int checkStatus = 0;

    int backgroundPid = arrayBackgroundPid[b];

    int p = waitpid(arrayBackgroundPid[b], &checkStatus, WNOHANG);

    // If this is true, the process hasn't exited or terminated
    if (p == 0 || p == -1){

      // kvalue will be 0 if the process is successfully terminated
      int kvalue = kill(backgroundPid, SIGTERM); // Sends the SIGTERM signal

      if (kvalue != 0){
        kill(backgroundPid, SIGKILL); //SIGKILL is used as a last resort because it is not catchable
      } 

    }
  }

}


// Takes two arguments
// a string named command
// an integer named posR
// and removes the redirector symbol at posR and the word after it
// For example, if command="ls junk > outfile" and posR=3, after running the function, command="ls junk"
// For example, if command="wc < infile" and posR=2, after running the function, command="wc"
void removeRedirection(char* command, int posR){

  char newCommand[2100]; empty(newCommand);
  char arg1[2100]; empty(arg1);

  for (int i = 1; i < getNumberCommandArguments(command)+1; i++){

    // Copies all arguments of command into newCommand except for the redirector symbol at posR and the argument after it
    if (i != posR && i != posR+1){

      char arg[2100]; empty(arg);
      getArgument(i, command, arg);

      // No space is inserted before the first argument
      if (strlen(newCommand) != 0){
        strcat(newCommand, " ");
      }
      strcat(newCommand, arg);

    }
  }

  empty(command);
  strcpy(command, newCommand);

}


int
main()
{


  int input_fd = STDIN_FILENO;
  int output_fd = STDOUT_FILENO;
  int copy_input_fd = dup(input_fd); // This is useful so that we can reverse any input redirection
  int copy_output_fd = dup(output_fd); // This is useful so that we can reverse any output redirection

  int shellpid = getpid();
  int lastStatus = 0; // Will contain the exit value or termination signal of last foreground process
  int arrayBackgroundPid[1000]; // An array that will contain the pids of background children processes
  int numberBackgroundPid = 0; // This will be the length of arrayBackgroundPid


  // Ignores SIGINT 
  struct sigaction ignore_SIGINT = {0};
  ignore_SIGINT.sa_handler = SIG_IGN;
  sigfillset(&ignore_SIGINT.sa_mask);
  ignore_SIGINT.sa_flags = 0;
  sigaction(SIGINT, &ignore_SIGINT, NULL);


  // Handles SIGTSTP. Toggles foregroundOnly mode upon receiving SIGTSTP
  struct sigaction SIGTSTP_parent = {0};
  SIGTSTP_parent.sa_handler = handle_SIGTSTP;
  sigfillset(&SIGTSTP_parent.sa_mask);
  SIGTSTP_parent.sa_flags = 0;
  sigaction(SIGTSTP, &SIGTSTP_parent, NULL);


  while (1){

    // Before prompting the user for input, we reverse any input/output redirection
    // performed in a previous command
    dup2(copy_input_fd,input_fd);
    dup2(copy_output_fd,output_fd);

    // Before prompting the user for input, checks if any background processes terminated and if so
    // displays a message containing the pid and exit value of terminated process
    messageTerminatedBackgroundChild(numberBackgroundPid, arrayBackgroundPid);


    // ----------------------------------- Input -----------------------------------

    // command contains the input of the user
    char command[2049] = "";  empty(command);
    int numberBytesRead = getInput(STDIN_FILENO, command);

    // ------------------------------ Processing Input ------------------------------


    // strippedCommand will later contain the command without the newline character at the end
    char strippedCommand[2049] = ""; empty(strippedCommand);
    // expandedCommand will later contain the command with expansions of any $$
    char expandedCommand[2100] = ""; empty(expandedCommand);
    // backgroundCommand will be set to 1 later if there is an & at the end
    int backgroundCommand = 0;
    // inputRedirected will be set to 1 later if there is input redirection
    int inputRedirected = 0;
    // outputRedirected will be set to 1 later if there is output redirection
    int outputRedirected = 0;


    // ------------- Part 1 of Processing Input -------------
    // Removing new line character
    // ------------------------------------------------------
    if (numberBytesRead > 0){

      // Since command contains a new line character at the end that we don't want to 
      // be part of the command we will stripping the new line character at the end of 
      // the command by copying only the number of bytes written into strippedCommand
      strncpy(strippedCommand,command,numberBytesRead);
      // Expanding all $$ symbols
      fullExpand(strippedCommand,expandedCommand,shellpid);
    }

    // ------------- Part 2 of Processing Input -------------
    // Toggling foregroundOnly mode
    // If in foregroundOnly mode, removes '&' if it exists
    // ------------------------------------------------------
    if (foregroundOnly > 2 && foregroundOnly %2 != 0 && numberBytesRead > 1){

      if (expandedCommand[strlen(expandedCommand)-1] == '&'){
        removeAmpersand(expandedCommand);
      }

    }

    // ------------- Part 3 of Processing Input -------------
    // Taking into account background commands
    // Removes ampersand symbol and sets backgroundCommand = 1 
    // ------------------------------------------------------
    if (expandedCommand[strlen(expandedCommand)-1] == '&'){

      backgroundCommand = 1;
      removeAmpersand(expandedCommand);

    }

    // ------------- Part 4 of Processing Input -------------
    // Redirecting input or output
    // ------------------------------------------------------
    inputRedirected = isInputRedirected(expandedCommand);
    outputRedirected = isOutputRedirected(expandedCommand);

    if (inputRedirected == 1){

      char ir[] = "<";
      int posIR = getArgumentPosition(ir,expandedCommand);

      // newInput will be the word that comes after <
      char newInput[2049]; empty(newInput);
      getArgument(posIR+1,expandedCommand,newInput);

      // Opens newInput
      int ifd = open(newInput, O_RDONLY, 0777);
      if (ifd == -1){
        w("cannot open ");
        w(newInput);
        w(" for input");
        n();
        lastStatus = 1;
        continue;
      }

      // Redirects input
      dup2(ifd,input_fd);
      // Removes the redirection symbol and the word coming after it
      removeRedirection(expandedCommand,posIR); 
    }

    if (outputRedirected == 1){

      char or[] = ">";
      int posOR = getArgumentPosition(or,expandedCommand);

      // newOutput will be the word that comes after <
      char newOutput[2049]; empty(newOutput);
      getArgument(posOR+1,expandedCommand,newOutput);

      // Opens newOutput
      int ofd = open(newOutput, O_WRONLY | O_TRUNC | O_CREAT, 0777); // If file exists, truncate it. If doesn't exist, create it.
      if (ofd == -1){
        w("cannot open ");
        w(newOutput);
        w(" for output");
        continue;
      }

      // Redirects output
      dup2(ofd,output_fd); 
      // Removes the redirection symbol and the word coming after it
      removeRedirection(expandedCommand,posOR);
    }


    // ---------------------------- Processing Output ----------------------------


    // Command is empty line
    if (numberBytesRead == 0){
      continue;

    // Command is a comment
    } else if (strncmp(expandedCommand, "#",1) == 0){
      continue;

    // Command is "exit"
    } else if (strncmp(expandedCommand, "exit",4) == 0 && numberBytesRead == 4) {
      
      // Kill all other processes started
      terminateBackgroundProcessses(numberBackgroundPid, arrayBackgroundPid);
      return 0;

    // Command is a "cd"
    // The two parts of the if statement are for zero or one argument to the cd command
    } else if ((strncmp(expandedCommand, "cd", 2) == 0 && numberBytesRead == 2)  || 
    (strncmp(expandedCommand, "cd ", 3) == 0 && getNumberCommandArguments(expandedCommand) == 2)){

      if (numberBytesRead == 2){
        // Changes directory to the home directory
        chdir(getenv("HOME"));

      } else {
        // Changes directory to the second argument in the command
        char newDirectory[2049] = ""; empty(newDirectory);
        getArgument(2,expandedCommand,newDirectory);
        chdir(newDirectory);
      }

      continue;


    // Command is "status"
    } else if (strncmp(expandedCommand, "status", 6) == 0) {

      // Case of last foreground process terminated by a signal
      if (lastStatus > 1){

        // Prints termination signal
        char stat[20] = ""; empty(stat);
        strcat(stat,"terminated by signal ");
        char statV[2] = ""; empty(statV);
        sprintf(statV, "%i", lastStatus);
        strcat(stat,statV);
        w(stat);
        n();

        continue;

      // Case of last foreground process exiting itself
      } else {

        // Prints exit value
        char stat[20] = ""; empty(stat);
        strcat(stat,"exit value ");
        char statV[2] = ""; empty(statV);
        sprintf(statV, "%i", lastStatus);
        strcat(stat,statV);
        w(stat);
        n();

        continue;
      }


    // Command will be forked off and executed with execv
    } else {


      if (numberBytesRead <= 0){
        continue;
      }

      // newarg is an array that will contain each argument of the command entered
      char **newarg;
      newarg = malloc(2600);

      int success = fillUpArgArray(newarg,expandedCommand);

      // If success is -1, the command entered wasn't found in any path
      if ( success == -1 ){
        char firstArg[2049]; empty(firstArg);
        getArgument(1,expandedCommand, firstArg);
        w(firstArg);
        w(": command not found");
        n();
        free(newarg);
        continue;
        
      }

      int childStatus;
      pid_t childPid = fork();

      // Unsuccessful fork
      if (childPid == -1){
        
        perror("fork()\n");
        exit(1);


      // Code run by child
      } else if (childPid == 0){

        // Makes all children ignore SIGTSTP 
        struct sigaction ignore_action_TSTP = {0};
        ignore_action_TSTP.sa_handler = SIG_IGN;
        sigaction(SIGTSTP, &ignore_action_TSTP, NULL);

        // Foreground child process
        if (backgroundCommand == 0) {

          // Setting up the SIGINT handling for the foreground child, which is
          // different from that of the parent
          // Any foreground child process will terminate upon receiving the SIGINT signal
          struct sigaction SIGINT_action = {0};
          SIGINT_action.sa_handler = SIG_DFL; // SIG_DFL can be inherited into execv
          sigaction(SIGINT, &SIGINT_action, NULL);

          // Runs the command
          execv(newarg[0], newarg);

          // If the execution of the wasn't successful, prints an error message
          char errorArg[2049]; empty(errorArg);
          getArgument(1,expandedCommand,errorArg);
          perror(errorArg);
          fflush(stdout);
          exit(1);

        // Background child process
        } else {

          // Background processes for which input hasn't been redirected are redirected to /dev/null
          if (inputRedirected == 0){
            int ifd = open("/dev/null", O_RDONLY);
            dup2(ifd,input_fd);
          }

          // Background processes for which out hasn't been redirected are redirected to /dev/null
          if (outputRedirected == 0){
            int ofd = open("/dev/null", O_WRONLY);
            dup2(ofd,output_fd);     
          }

          // Runs the command
          execv(newarg[0], newarg);

          // If the execution of the wasn't successful, prints an error message
          char errorArg[2049]; empty(errorArg);
          getArgument(1,expandedCommand,errorArg);
          perror(errorArg);
          fflush(stdout);
          exit(1);

        }


      // Code run by parent
      } else {

        // newarg has no use at this point so it is freed up
        free(newarg);

        // Returns the input and output to the original stdin and stdout
        dup2(copy_input_fd,input_fd);
        dup2(copy_output_fd,output_fd);

        // If the command run was run as a child foreground process
        if (backgroundCommand == 0){

          // Waits for the child foreground process to continue
          childPid = waitpid(childPid, &childStatus, 0);

          // Case of the child exiting itself
          if (WIFEXITED(childStatus)){

            // Stores the exit value in lastStatus which is defined outside the while loop
            lastStatus = WEXITSTATUS(childStatus);
          }

          // Case of the child being terminated by a signal
          // Prints a message of the child termination signal
          if (WIFSIGNALED(childStatus)){

            lastStatus = WTERMSIG(childStatus);

            w("terminated by signal ");

            char signumber[4]; empty(signumber);
            sprintf(signumber, "%i", WTERMSIG(childStatus));
            w(signumber);
            n();

          }

        // If the command run was run as a child background process
        } else {

          // childprocessID will contain the child pid
          char childprocessID[10];  empty(childprocessID);
          sprintf(childprocessID, "%i", childPid);    

          // Prints required message
          w("background pid is ");
          w(childprocessID);
          n();

          // Appends the pid of the child background process to arrayBackgroundPid
          // Increments numberBackgroundPid
          arrayBackgroundPid[numberBackgroundPid] = childPid;
          numberBackgroundPid ++;

          // We don't wait for the background process to finish
          // using WNOHANG
          childPid = waitpid(childPid, &childStatus, WNOHANG);

        }
        
      }
    }
      continue;
   
  }

}
