/*
AUTHOR: Joshua Strozzi
PROGRAM: smallsh
CLASS: CS-344
DUE: Nov 14 - 11:59PM
*/
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXINPUT 2048
#define MAXARGS  512

char** getUserInput(int*);
int execCommand(char**, char*, char*, int, int*, struct sigaction);
int findInputOuput(char**, char*, char*, int);
int isForeground(char**, int*);
void expansion(char**, int, int);

void statusFunk(int*);

pid_t pidArr[100];                                                              //array of background proc pids to be
int numBackground;                                                              // num of occupied indexes

int allowBackground=1;
int exitStatGlob = -5;

void catchSIGINT(int signo)
{
  char* message = "Terminated by ^C\n";
  write(STDOUT_FILENO, message, 28);
  fflush(stdout);
}

void catchSIGUSR(int signo)
{
  if(allowBackground==1){                                                       //if not in foreground only mode
    char* message = "Entering foreground only mode\n";
    write(STDOUT_FILENO, message, 30);
    fflush(stdout);
    allowBackground =0;                                                         //set allowBackground bool to false
  }
  else if(allowBackground==0){                                                  //if in foreground only mode
    char* message = "Exiting foreground only mode\n";
    write(STDOUT_FILENO, message, 30);
    fflush(stdout);
    allowBackground =1;                                                         //set allowBackground bool to true
  }
}

int main(){

  char**  arguments;                                                            //a 512 length array of char* so we can point to up to 512 arguments
  //char*   input, *output;
  int     numArgs, fileIO;
  int     lastExitStat=-5;
  int     i;
  char    input[100], output[100];
  int     foreground;
  pid_t   currPid;
  int     pid = getpid();

  //signal handling stuff: https://oregonstate.instructure.com/courses/1725991/pages/3-dot-3-advanced-user-input-with-getline

  struct sigaction SIGINT_action = {0}, SIGUSR2_action = {0}, ignore_action = {0};//completely initialize complicated struct to be empty
  //CTRL-C
  SIGINT_action.sa_handler = catchSIGINT;                                       //set sa_handler=catchSIGINT
  SIGINT_action.sa_flags = SA_RESTART;//set sa_flags
  sigfillset(&SIGINT_action.sa_mask);//sigfillset(&s.sa_mask)
  sigaction(SIGINT, &SIGINT_action, NULL);//sigaction(&mystruct)

  //CTRL-Z
  SIGUSR2_action.sa_handler = catchSIGUSR;                                      //set sa_handler=catchSIGUSR
  SIGUSR2_action.sa_flags = SA_RESTART;//set sa_flags
  sigfillset(&SIGUSR2_action.sa_mask);//sigfillset(&s.sa_mask)
  sigaction(SIGTSTP, &SIGUSR2_action, NULL);//sigaction(&mystruct)



  while(1){
    fileIO=0;
    //check background procs for reaping
    for(i =0; i<numBackground; i++){                                            //for every process id in pid array
      currPid=pidArr[i];                                                        //why did i do this twice?
      if(currPid>0){                                                            //if pid has not already been reaped
        currPid=pidArr[i];                                                      //why did I do this twice?

        //if(waitpid(pidArr[i], &lastExitStat, WNOHANG)){
        if (waitpid(pidArr[i], &exitStatGlob, WNOHANG)){                        //check pid, if not done, exit, else reap
          int exitStatus = WEXITSTATUS(exitStatGlob);
          printf("Background Pid %d done, exit status was: %d\n", currPid, exitStatus);
          pidArr[i]=0;                                                          //set pid at curr position in pid array to zero so it doesn't get checked and reaped again

          if(WIFSIGNALED(exitStatGlob)!=0){
            int exitSig = WTERMSIG(exitStatGlob);
            printf("Background Pid %d done, signal was %d\n", currPid, exitSig);
            fflush(stdout);
          }



        }
      }
    }
    //end  reaping

    arguments =  getUserInput(&numArgs);                                        //set up arguments array
    expansion(arguments, numArgs, pid);                                         //expand all instances of "$$"

    //COMMENT
    if (arguments[0][0] == '#' || arguments[0][0] == '\0') {                    //if comment, do nothing
      continue;
    }
    //STATUS
    else if(strcmp(arguments[0], "status")==0){
    //prExitStat
    statusFunk(&lastExitStat);
    }
    //EXIT
    else if(strcmp(arguments[0], "exit") == 0){
      break;
    }
    //CD
    else if(strcmp(arguments[0], "cd")==0){
      if(!arguments[1]){                                                        //if directory not specified
        chdir(getenv("HOME"));                                                  //by default go do home dir if no dir is given
      }
      else if(chdir(arguments[1])==-1){                                         //if directory not accessible

        perror("Directory not accessible from here\n");                         //getting more output than I wanted
        fflush(stdout);
      }
    }
    //Anything else
    else{
      foreground=isForeground(arguments, &numArgs);                             //retuns bool for if function is foreground
      //find any input and output files
      fileIO=findInputOuput(arguments, input, output, numArgs);                 //set up input and output files up if they exist...didn't actually end up needing to use fileIO

      //execute!
      execCommand(arguments, input, output, foreground, &lastExitStat, SIGINT_action);
    }




  }

  printf("Thanks for using smallsh by Joshua Strozzi\n");

  return 0;
}


/***************************************************************
 * 			int expansion(char**, int, int)
 *
 * checks array of args for pid call: "$$"
 *
 * PARAMS:
 *		char** Arguments - the entire command parsed into arguments
 *    int numArgs - current number of arguments in arg array
 *    int parentPid - pid of main process
 *	OUTPUT:
 *		void - N/A
 *
 *  POST-CONDITIONS:
 *    any instance of $$ is expanded to the current pid of that process
 *
 *
 *
 ***************************************************************/
void expansion(char** argArray, int numArgs, int parentPid){
  int i, instance;
  char newString[100], temp[50];
  memset(newString,'\0', sizeof(char)*100);

  char* currArg, *currInstance;


  for(i=1;i<numArgs; i++){
    currInstance=argArray[i];                                                   //set pointer to beginning of argument
    currArg=argArray[i];                                                        //repeat for point that holds beginning of new word to add
    instance = 0;
    if( (currInstance=strstr(currArg, "$$"))!=NULL ){                           //if strstr sets a pointer, set instance bool to true
      instance=1;
    }
    while( (currInstance=strstr(currArg, "$$"))!=NULL ){                        //while you can set pointer to beginning of instance of "$$"
      memset(temp, '\0', sizeof(temp));                                         //clear out temp string
      strncpy(temp, currArg, (strlen(currArg) - strlen(currInstance) ));        //copy data of string until next instance of "$$" into temp
      sprintf(newString ,"%s%s%d",newString, temp, parentPid);                  //save newstring up until this point, with new word and add pid where "$$ is"
      currInstance=currInstance+2*sizeof(char);                                 //iterate to location following "$$"
      currArg=currInstance;                                                     //do the same for this pointer which points to beginning of new words
    }

    if(instance){                                                               //if instance bool,
      argArray[i]=malloc(sizeof(newString));                                    //set curr arg char* to a memory allocated new string
      strcpy(argArray[i], newString);
    }
  }

}



/***************************************************************
 * 			int statusFunk(int*)
 *
 * prints exit status of process last exited
 *
 * PARAMS:
 *		int* childExitStatus - not actually used, instead I use the global variable exitStatGlob
 *	OUTPUT:
 *		void - N/A
 *
 *  POST-CONDITIONS:
 *    print the exit status of last process
 *
 *
 *
 ***************************************************************/
void statusFunk(int *childExitStatus){

  if (WIFEXITED(exitStatGlob)!=0){                                              //http://web.engr.oregonstate.edu/~brewsteb/CS344Slides/3.1%20Processes.pdf
    printf("The process Exited, ");
    fflush(stdout);
    int exitStatus = WEXITSTATUS(exitStatGlob);
    printf("exit status was %d\n", exitStatus);
    fflush(stdout);

  }
  if(WIFSIGNALED(exitStatGlob)!=0){
    int exitSig = WTERMSIG(exitStatGlob);
    printf("Child terminated by signal: %D\n", exitSig);
    fflush(stdout);
  }
}





/***************************************************************
 * 			int isForeground(char**)
 *
 * checks array of args for ampersand: "&"
 *
 * PARAMS:
 *		char** Arguments - the entire command parsed into arguments
 *    int* numArgs - current number of arguments in arg array
 *	OUTPUT:
 *		bool for if process is to be foreground or background
 *
 *  POST-CONDITIONS:
 *    foreground bool is set
 *
 *
 *
 ***************************************************************/
int isForeground(char** arguments, int* numArgs){
  if( strcmp(arguments[*numArgs-1], "&")==0){                                   //if last argument is ampersand
    arguments[*numArgs-1]=NULL;                                                 //remove ampersand from argument array
    *numArgs-=1;                                                                //subtract argsNum by one
    return 0;                                                                   //set foreground to false
  }

  return 1;                                                                     //if no ampersand, its foreground
}




/***************************************************************
 * 			void execCommand(char**)
 *
 * Executes hardcoded commands or calls bash commands
 *
 * PARAMS:
 *		char** Arguments - the entire command parsed into arguments
 *    char * inName - input file name
 *    char* outName - output file name
 *    int foreground - bool for if proc is Foreground
 *    int* childExitStats - self explanatory, but now unnecessary cuz I use a glob variable
 *    struct sigaction SigAc - signal handling variable
 *	OUTPUT:
 *		void - NA
 *
 *  POST-CONDITIONS:
 *    process is forked and completed for user
 *
 *
 *
 ***************************************************************/
int execCommand(char** arguments, char* inName, char* outName, int foreground, int* childExitStatus,   struct sigaction sigintAc){

  int inputFD, outputFD, redirect;
  pid_t spawnpid = -5;


  spawnpid=fork();                                                              //helpful: http://web.engr.oregonstate.edu/~brewsteb/CS344Slides/3.1%20Processes.pdf
  switch(spawnpid){
    case -1:                                                                    //if spawn pid failed
      perror("Hull breach!");
      exit(1);
      break;

    case 0:                                                                     //if spawn pid worked


      //redirect input

      if(inName[0]!='\0'){                                                      //http://web.engr.oregonstate.edu/~brewsteb/CS344Slides/3.4%20More%20UNIX%20IO.pdf
        inputFD = open(inName, O_RDONLY);                                       //set input file descriptor to file pointer
        if(inputFD==-1){
          perror("open() inputFD\n");
          exit(1);
        }

        redirect = dup2(inputFD, 0);                                            //redirect stdin pointer to file
        if(redirect==-1){
          perror("dup2() inputFD\n");
          exit(2);
        }
          fcntl(inputFD, F_SETFD, FD_CLOEXEC);                                  //  http://web.engr.oregonstate.edu/~brewsteb/CS344Slides/3.4%20More%20UNIX%20IO.pdf (page 16)

      }
      //redirect output
      if(outName[0]!='\0'){
        outputFD = open(outName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if(outputFD == -1){
          perror("open() outputFD\n");
          exit(1);
        }

        redirect = dup2(outputFD, 1);                                           //redirect stdout pointer thing to file
        if(redirect == -1){
          perror("dup2() outputFD\n");
          exit(2);
        }

        fcntl(outputFD, F_SETFD, FD_CLOEXEC);                                   //  http://web.engr.oregonstate.edu/~brewsteb/CS344Slides/3.4%20More%20UNIX%20IO.pdf (page 16)
      }


      if( (outName[0]=='\0') && !foreground){//experiment
        dup2(open("/dev/null", O_WRONLY), 1);
      }
      if( (inName[0]=='\0') && !foreground){
        dup2(open("/dev/null", O_RDONLY), 0);

      }
      //now execute
      if( execvp(arguments[0], /*( char* const* )*/arguments) == -1){           //execute users call, which replaces my code in this process with it's code, if it fails, print error
        perror("execvp()\n");
        fflush(stdout);
        exit(2);
      }

      //exit(0);
      break;

    default:
      //based off of skeleton code shown by brewster (except for allowBackground, that was my freaking brain, bro...sorry really hyped about it right now)
  /*    if(foreground || allowBackground==0){                                                     //does foreground processes and runs ampersand processes if it's in foreground only mode      //wait for child to terminate: waitpid()
        //set sigint back to default
        sigintAc.sa_handler =SIG_DFL;                                              //^C gets default behavior
        sigaction(SIGINT, &sigintAc, NULL);
        fflush(stdout);
        //pid_t childPid=waitpid(spawnpid, childExitStatus, 0);                                 //lecture 3.2
        pid_t childPid=waitpid(spawnpid, &exitStatGlob, 0);
      }
      else if (allowBackground==1){                                                             //if not in foreground only mode, run processes in the background, store pid to be reaped when done
        int devNull = open("/dev/null", O_RDONLY);
        dup2(devNull, 0);                                                                     //https://stackoverflow.com/questions/14846768/in-c-how-do-i-redirect-stdout-fileno-to-dev-null-using-dup2-and-then-redirect

        //dup2(open("/dev/null", O_RDONLY), 0);

        fcntl(devNull, F_SETFD, FD_CLOEXEC);
        pidArr[numBackground++]=spawnpid;

        printf("Background pid is %d\n", spawnpid);
      }*/
      break;
  }
  //executed by both
  if(foreground || allowBackground==0){                                                     //does foreground processes and runs ampersand processes if it's in foreground only mode      //wait for child to terminate: waitpid()
        //set sigint back to default
    sigintAc.sa_handler =SIG_DFL;                                              //^C gets default behavior
    sigaction(SIGINT, &sigintAc, NULL);
    fflush(stdout);
        //pid_t childPid=waitpid(spawnpid, childExitStatus, 0);                                 //lecture 3.2
    pid_t childPid=waitpid(spawnpid, &exitStatGlob, 0);
  }
  else if (allowBackground==1){                                                             //if not in foreground only mode, run processes in the background, store pid to be reaped when done
  //  int devNull = open("/dev/null", O_RDONLY);
  //  dup2(devNull, 0);                                                                     //https://stackoverflow.com/questions/14846768/in-c-how-do-i-redirect-stdout-fileno-to-dev-null-using-dup2-and-then-redirect

  //dup2(open("/dev/null", O_RDONLY), 0);

  //  fcntl(devNull, F_SETFD, FD_CLOEXEC);
    pidArr[numBackground++]=spawnpid;

    printf("Background pid is %d\n", spawnpid);
  }

  return 0;

}


/***************************************************************
 * 			void findInputOuput(char**, char[], char[], int)
 *
 * in the event there are carrots (< or >), it stores following arg into either input[] or output[]
 *
 * PARAMS:
 *		char** arguments - user command parsed into argument Array
 *    char* input -  array that could hold string for input files
 *    char* output - same as last param, but output string
 *    int numargs - take a guess
 *	OUTPUT:
 *		int 1 or 0 bool that I don't need, but whatever
 *
 *  POST-CONDITIONS:
 *    input and / or output char array is full
 *
 *
 *
 ***************************************************************/
int findInputOuput(char** args, char* input, char* output, int numArgs){
  int i, fileIO=0;
  memset(input, '\0', sizeof(char)*100);                                        //make sure input and output is empty
  memset(output, '\0', sizeof(char)*100);
  for(i=0;i<numArgs; i++){
    if(strcmp(args[i], "<")==0){                                                //if input carrot
      args[i]=NULL;
      strcpy(input , args[++i]);                                                //copy string in next position into input
      args[i]=NULL;
      fileIO=1;
    }
    else if(strcmp(args[i], ">")==0){                                           //if output carrot
      args[i]=NULL;
      strcpy(output , args[++i]);                                               //copy string in next position into output char array
      args[i]=NULL;
      fileIO=1;
    }
  }

  return fileIO;                                                                //bool for if there is fileIO
}





/***************************************************************
 * 			void getUserInput(char**)
 *
 * Prompts the user and parses the input into an array of arguments
 *
 * PARAMS:
 *		int* numArgs - number of arguments
 *	OUTPUT:
 *		char** arguments - all the arguments
 *
 *  POST-CONDITIONS:
 *    Array of arguments set up
 *
 *
 *
 ***************************************************************/
char** getUserInput(int* numArgs){

  char*  userCommand;
  size_t bufferSize = MAXINPUT*sizeof(char);
  userCommand = malloc(MAXINPUT * sizeof(char));                                //don't free, this is referenced by our char** array
  memset(userCommand, '\0', sizeof(char*)*MAXINPUT);
  if(userCommand ==NULL){
    perror("failed to allocate for user input\n");
    exit(1);
  }


  //Get User Input                                                              //Helpuful Link: https://oregonstate.instructure.com/courses/1725991/pages/3-dot-3-advanced-user-input-with-getline

  while(1){
    printf(": ");
    fflush(stdout);
    getline(&userCommand, &bufferSize, stdin);
    if(strlen(userCommand)-1 ==0){                                              //if user input is length of 0, keep asking for input
      clearerr(stdin);
    }
    else                                                                        //otherwise, we've got the commands we need
      break;
  }



  if(strlen(userCommand)>0){
    userCommand[strlen(userCommand)-1]='\0';                                    //get rid of newLine inserted by getline function
  }


 /********************* Parse ****************************/                     //Helpful: https://stackoverflow.com/questions/18927793/how-to-use-strtok
  char** arguments = malloc(sizeof(char*) * MAXARGS);
  char*  argument=userCommand;


  int   currArg=0;                                                              //keeps track of positiong in arg array for inserting arguments

  while((argument = strtok(argument, " ")) !=NULL){
    arguments[currArg++] = argument;                                            //put argument into arguments array and iterate the currArg counter
    argument = NULL;                                                            //idk I got it from the helpful link above
  }
  arguments[currArg] = NULL;                                                    //make sure to set the array of numargs+1 to null

  *numArgs = currArg;
//  free(userCommand);                                                          //doing this breaks my code

return arguments;

}
