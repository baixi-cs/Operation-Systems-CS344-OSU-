/*
AUTHOR: Joshua Strozzi
PROGRAM: adventure program
CLASS: CS-344
DUE: OCT 25 - 11:59PM
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <pthread.h>
#include <fcntl.h>

pthread_mutex_t timeMutex = PTHREAD_MUTEX_INITIALIZER;


struct room {
  char roomName[9];
  int   numConnections;       //at least 3, at most 6
  int   type;                 //START_ROOM, END_ROOM, or MID_ROOM (only 1 of each of the first 2)
  char connectionNames[6][9];
};


void changeCurrWorkingDirectory();                  //moves current working directory into folder with room files
struct room* listFiles_experiment();                //sets up room struct array and passes it back out
void addConnection(char**, char*, int);             //
void printFunction(const struct room*);

void theGame( struct room*, int, int);
int findStart(const struct room*);
int findEnd(const struct room*);
void printConnections(const struct room*, int);
int isRoom(const struct room*, const char*, int);
void printSteps(char**, int);
void* timeThreadFunc(void*);
void printTimeFile();

int main(){


//________________SET GAME NECESSARY INFO________________//
  struct room* roomGrid;
  changeCurrWorkingDirectory();                           ///do this before EVERYTHING ELSE~!!!!!!!!!!
  roomGrid=listFiles_experiment();                        //initialize room array
  chdir("..");                                            //move us back out to the directory our program was first initialized
//  printFunction(roomGrid);
//________________BEGIN ________________//
  theGame(roomGrid, findStart(roomGrid), findEnd(roomGrid));

  pthread_mutex_destroy(&timeMutex);
  free(roomGrid);
  return 0;
}

/********************************************************************************
* Description: Displays current room, where you can go, and upon completion shows you steps
* Parameters: the struct room array filled with all the room info read in from the files
* Returns: nothing
* Pre-Conditions: Room array has been filled with info
* Post-Conditions: Game has been completed
*********************************************************************************/
void theGame( struct room* roomArray, int startroom, int endroom){         //pass grid as read-only object for gamePlay
  int curr_room=startroom, steps=0, nextRoom;
  char whereto[12];
  char* roomsVisted[100];
  char timeBuffer[50];
  //________________SET UP MUTEX STUFF________________//  source: https://piazza.com/class/jlcyadqbpv1of?cid=233


  pthread_mutex_lock(&timeMutex);
  //set up time thread
  int timeThread;
  pthread_t thread;
  timeThread=pthread_create(&thread, NULL, timeThreadFunc, NULL);

//________________________Begin Game____________________________??

  while(curr_room != endroom){                                        //while not in end room, do this
    memset(whereto, '\0', sizeof(whereto));                           //make sure the user input buffer string is initiallized with null terminators
    printf("CURRENT LOCATION: %s\n", roomArray[curr_room].roomName);
    printf("POSSIBLE CONNECTIONS: ");
    printConnections(roomArray, curr_room);                           //print possible rooms to chosenRoomNames
    printf("WHERE TO? >");
    if(fgets(whereto, sizeof(whereto), stdin)){                       //read in users input into "whereto"
      whereto[strlen(whereto)-1]='\0';
      printf("\n");
      nextRoom=isRoom(roomArray, whereto, curr_room);                           //nextRoom wiill get index for next room or -1 for invalid input, or -2 for time
      if(nextRoom==-1){
          printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
      }
      else if(nextRoom == -2){

        pthread_mutex_unlock(&timeMutex);
        pthread_join(thread,NULL);
        pthread_mutex_lock(&timeMutex);
        timeThread = pthread_create(&thread, NULL, timeThreadFunc,NULL);
        //display time to screen by reading from file made by timeThreadFunc
        printTimeFile();                                                         //function for printing contents of currentTime.txt

      }
      else{
        roomsVisted[steps]=roomArray[nextRoom].roomName;                      //array keeping track of steps taken gets next room name
        curr_room=nextRoom;                                                   //set the current room tracker to the next room
        steps+=1;                                                             //increment steps taken counter
      }
    }
  }

  printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\nYOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", steps);
  printSteps(roomsVisted, steps);                                               //function printing what rooms have been visited

  pthread_mutex_destroy(&timeMutex);
}


/********************************************************************************
* Description: called when joining the thread with the timeMutex thread, puts local time in a new file
* Parameters: none
* Returns: nothing
* Pre-Conditions: timeMutex thread has been recreated or something
* Post-Conditions: new file holding local time
*********************************************************************************/

void* timeThreadFunc(void* garbage){
  pthread_mutex_lock(&timeMutex);

  char writeBuffer[50];
  int file_descriptor;
  ssize_t nwritten;
  time_t currTime;
  time(&currTime);                                                 //https://stackoverflow.com/questions/5141960/get-the-current-time-in-c
  struct tm* temp = localtime(&currTime);
//  printf("time = \"%s\"\n", asctime(temp));
  strftime(writeBuffer, sizeof(writeBuffer), "%l:%M%P, %A, %B %d, 20%y\n", temp); 
    //printf("time = \"%s\"\n", writeBuffer);
  file_descriptor=open("currentTime.txt", O_WRONLY | O_CREAT, 0600);

  nwritten=write(file_descriptor, writeBuffer, strlen(writeBuffer)*sizeof(char));
  close(file_descriptor);




  pthread_mutex_unlock(&timeMutex);
}

/********************************************************************************
* Description: prints out contents of file made by timeThreadFunc "currentTime.txt"
* Parameters: none
* Returns: nothing
* Pre-Conditions: currentTime.txt has been made and filled
* Post-Conditions: contents of currentTime.txt have been printed
*********************************************************************************/
void printTimeFile(){

  FILE* myFile;                                     //file pointer

  char* curr_line;                                  //sets strings read from file to string literals to be read out
  ssize_t nread;                                    //current line pointer or something
  size_t len=0;

  int newestDirTime = -1;                           // Modified timestamp of newest subdir examined
  char targetDirPrefix[32] = "currentTime";        // Prefix we're looking for
  //char readBuffer[50];


  DIR* dirToCheck;                          // Holds the directory we're starting in
  struct dirent *fileInDir;                  // Holds the current subdir of the starting dir
  struct stat dirAttributes;                  // Holds information we've gained about subdir

  dirToCheck = opendir("."); // Open up the directory this program was run in

  if (dirToCheck > 0) // Make sure the current directory could be opened
  {
    while ((fileInDir = readdir(dirToCheck)) != NULL) // Check each entry in dir
    {
      if (strstr(fileInDir->d_name, targetDirPrefix) != NULL) // If entry has prefix
      {

        //printf( "%s exists\n", fileInDir->d_name);
        myFile = fopen(fileInDir->d_name, "r");
        while(nread = getline(&curr_line, &len, myFile)!=-1){             //while can read file into curr_line
          printf("%s\n", curr_line);                                    //print contents of curr_line
        }


      }
    }
  }

  closedir(dirToCheck);                             // Close the directory we opened
}




/********************************************************************************
* Description: prints out contents of roomsvisited array
* Parameters: rooms visited array and number of steps taken
* Returns: nothing
* Pre-Conditions: game has been completed
* Post-Conditions: rooms visited array is printed out
*********************************************************************************/

void printSteps(char** stepsArray, int stepsTaken){                             //print away of all rooms visited
  int curr_step;

  for(curr_step=0; curr_step<stepsTaken; curr_step++){
    printf("%s\n",stepsArray[curr_step]);
  }

}



/********************************************************************************
* Description: functions reads users input, compares it to rooms connected to current room and all rooms that exists in order to return the room index they wish to go to
* Parameters: the room array, the users input, and the index of the users current room
* Returns: index of the room they are hoping to visit
* Pre-Conditions: room array exists, user has input a potential room name
* Post-Conditions: the user's next room value is either set or not
*********************************************************************************/

int isRoom(const struct room* roomArray, const char* input, int curr_room){
  int curr_connect, exists;
  int roomLocation;
  if(strcmp(input, "time")==0){
    return -2;
  }
  //                                   this loop can check for it room is connected
  for(curr_connect=0;curr_connect<roomArray[curr_room].numConnections;curr_connect++){    //for number of connections current room has
    if(strcmp(input, roomArray[curr_room].connectionNames[curr_connect])==0){             //if user input matches names of rooms connected to current room

      exists=1;           //flip on the exists bool
    }
  }

  if(exists!=1){                        //if the room doesnt exist in the array of connections of that room
    return -1;
  }
  //                                   // another loop for checking where room exists in roomstruct array
  for(curr_room=0;curr_room<7;curr_room++){
    if(strcmp(input, roomArray[curr_room].roomName)==0){
      return curr_room;
    }
  }
}



/********************************************************************************
* Description: prints out the curr rooms possible connections
* Parameters: the room array, and the index of the current room
* Returns: nothing
* Pre-Conditions: user is playing the game and is in a room
* Post-Conditions: possible connections printed
*********************************************************************************/

void printConnections(const struct room* roomArray, int curr_room) {

  int curr_connect;
  for(curr_connect=0; curr_connect <roomArray[curr_room].numConnections;curr_connect++){
    printf("%s", roomArray[curr_room].connectionNames[curr_connect]);
    if (curr_connect<(roomArray[curr_room].numConnections-1)){
      printf(", ");
    }
    else{
      printf(".");
    }
  }
  printf("\n");

}




/********************************************************************************
* Description: returns start room index
* Parameters: room array
* Returns: the room with type: START_ROOM
* Pre-Conditions:  room array intialized
* Post-Conditions: start room index in outside function set
*********************************************************************************/

int findStart(const struct room* roomArray){
  int start, curr_room;

  for(curr_room=0; curr_room<7; curr_room++){
    if(roomArray[curr_room].type==0){
      start=curr_room;
    }
  }
  return start;
}


int findEnd(const struct room* roomArray){
  int end, curr_room;

  for(curr_room=0; curr_room<7; curr_room++){
    if(roomArray[curr_room].type==2){
      end=curr_room;
    }
  }
  return end;
}




/********************************************************************************
THIS FUNCTION IS NO LONGER IS USE
*********************************************************************************/
void printFunction(const struct room* roomArray){
  int curr_room, curr_connection;
  for (curr_room=0;curr_room<7;curr_room++){
      printf("struct curr_room name: %s, with %d connections %d\n", roomArray[curr_room].roomName, roomArray[curr_room].numConnections, roomArray[curr_room].type);
      for(curr_connection=0; curr_connection< roomArray[curr_room].numConnections; curr_connection++){
        printf("\tconnection %d: %s\n", curr_connection+1, roomArray[curr_room].connectionNames[curr_connection]);
      }
      printf("\n");
  }
}




/********************************************************************************
* Description: Changes current working director to one filled with room files so they can be read into room structs
* Parameters: none
* Returns: nothing
* Pre-Conditions: buildroom program has been run
* Post-Conditions: the cwd is the room folder
*********************************************************************************/
void changeCurrWorkingDirectory() {                   //code provided by Ben Brewster here: https://oregonstate.instructure.com/courses/1725991/pages/2-dot-4-manipulating-directories
  int newestDirTime = -1;                           // Modified timestamp of newest subdir examined
  char targetDirPrefix[32] = "strozzij.rooms.";        // Prefix we're looking for
  char newestDirName[256];                           // Holds the name of the newest dir that contains prefix
  memset(newestDirName, '\0', sizeof(newestDirName));

  DIR* dirToCheck;                          // Holds the directory we're starting in
  struct dirent *fileInDir;                  // Holds the current subdir of the starting dir
  struct stat dirAttributes;                  // Holds information we've gained about subdir

  dirToCheck = opendir("."); // Open up the directory this program was run in

  if (dirToCheck > 0) // Make sure the current directory could be opened
  {
    while ((fileInDir = readdir(dirToCheck)) != NULL) // Check each entry in dir
    {
      if (strstr(fileInDir->d_name, targetDirPrefix) != NULL) // If entry has prefix
      {

        stat(fileInDir->d_name, &dirAttributes); // Get attributes of the entry

        if ((int)dirAttributes.st_mtime > newestDirTime) // If this time is bigger
        {
          newestDirTime = (int)dirAttributes.st_mtime;
          memset(newestDirName, '\0', sizeof(newestDirName));
          strcpy(newestDirName, fileInDir->d_name);

        }
      }
    }
  }

  closedir(dirToCheck);                             // Close the directory we opened


  printf("Newest entry found is: %s\n", newestDirName);
  chdir(newestDirName);


}



/********************************************************************************
* Description: reads all room files filles heap allocated array of room structs with their info
* Parameters: nothing
* Returns: heap allocated array of room structs
* Pre-Conditions:  no room array
* Post-Conditions: room array is rilled with file info and past back out for later use
*********************************************************************************/
struct room* listFiles_experiment(){
  struct room* roomArray= malloc(7*sizeof(struct room));


  char targetFileSuffix[6] = "_room";                           //suffix we're looking for

  DIR* dirToCheck;                                          // Holds the directory we're starting in
  struct dirent *fileInDir;                                    // Holds the current subdir of the starting dir
  struct stat dirAttributes;                                    // Holds information we've gained about subdir

  int file_descriptor, curr_room=0;

  ssize_t nread;
  size_t len = 0;


  FILE* myFile;

  char garbage1[12],garbage2[12], info[20];
  char* curr_line = NULL;

  dirToCheck=opendir(".");
    if (dirToCheck > 0)                                                 // Make sure the current directory could be opened
    {
      while ((fileInDir = readdir(dirToCheck)) != NULL)                  // Check each entry in dir
      {
        if (strstr(fileInDir->d_name, targetFileSuffix) != NULL)       // If entry has suffix "_room"
        {
          roomArray[curr_room].numConnections=0;                      //initialize struct variables
          roomArray[curr_room].type=0;
          myFile=fopen(fileInDir->d_name, "r");
          while(nread = getline(&curr_line, &len, myFile) != -1){   //while I can read file line into curr_line

            sscanf(curr_line, "%s %s %s", garbage1, garbage2, info);       //parse out string read from file into 3 different char arrays for comparing

            if (strcmp(garbage2, "NAME:")==0) {                           //if this line is holding name info

              strcpy(roomArray[curr_room].roomName, info );
            }
            if (strcmp(garbage1, "CONNECTION")==0){                          //if line holds connection info

              strcpy( roomArray[curr_room].connectionNames[roomArray[curr_room].numConnections], info);   //put connection name at connectionarray[current number of connections]

              roomArray[curr_room].numConnections+=1;                         //iterate connections num
            }
            if (strcmp(garbage2, "TYPE:")==0){                              //if curr_line holds room type
              if (strcmp(info, "MID_ROOM")==0){
                roomArray[curr_room].type=1;
              }
              else if (strcmp(info, "END_ROOM")==0){
                roomArray[curr_room].type=2;
              }
              else if (strcmp(info, "START_ROOM")==0){
                roomArray[curr_room].type=0;
              }
            }
          }
          fclose(myFile);
          curr_room++;
        }
      }

    }
    free(curr_line);
    closedir(dirToCheck);                                                         // Close the directory we opened

    return roomArray;                                                               //pas back out the array

}
