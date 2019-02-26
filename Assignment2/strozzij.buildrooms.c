/*
AUTHOR: Joshua Strozzi
PROGRAM: rooms program
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

#include <fcntl.h>

struct room {
  int   id;
  char* roomName;
  int   numConnections;       //at least 3, at most 6
  int   type;                 //START_ROOM, END_ROOM, or MID_ROOM (only 1 of each of the first 2)
  struct room* connections;   //if A has connect to B, then B must have connect to A, A cant connect to A, and A cant connect to both B and B    (assigned randomly each time program is run)
};

struct myNames {
  char* name;
};

const struct myNames name_array[10] = {
  { .name = "firepit" },
  { .name = "painroom" },
  { .name = "chamber" },
  { .name = "dankarea" },
  { .name = "Kitchen" },
  { .name = "Graveyrd" },
  { .name = "thevoid" },
  { .name = "Arena" },
  { .name = "Kiln" },
  { .name = "theDorms"}
};

int* determineTypes();
int* determineNames();
int** InitializeConnectionsTable();
void printConnectsArray(int**, int);
void DisplayAllInfo(int**, int*, int*);
void OutputInfotoFiles(int**, int*, int*);
int isArrayFull(int**);
int findConnect(int**, int);

void testFunc();

int main(){
  srand(time(0));
  int*  chosenRoomNames; //Indexes of above name_array
  int*  chosenRoomTypes;
  int** outboundConnections; //[0][0] will not be used

  int i;
  chosenRoomNames = determineNames();                   //names given in an array[0-6] with numbers 0-9
  outboundConnections = InitializeConnectionsTable();   //connections given in array[8][8] wherein [n][m] represents the connection between two rooms...n and m (not including [0][m] and [n][0])
  chosenRoomTypes = determineTypes();                   //types of rooms given in an array[6] wherein array[0]==1 means room 1 is a MID_ROOM


  OutputInfotoFiles(outboundConnections, chosenRoomNames, chosenRoomTypes);   //function for outputing info into the files

  for (i=0; i<8; i++){
    free(outboundConnections[i]);
  }
  free(outboundConnections);

  free(chosenRoomNames);
  free(chosenRoomTypes);

  return 0;
}


/********************************************************************************
* Description: does as named, ouputs info created into files
* Parameters: the connections array, the array of name indexes, and arry of types
* Returns: nothing
* Pre-Conditions: no files exist with room info
* Post-Conditions: new files holding room info
*********************************************************************************/
void OutputInfotoFiles(int** connections, int* names, int* types){
  //    MAKE DIRECTORY     //
  char dirName[25];                      //buffer size of 25
  int pid = getpid();                     //suffix of file names
  sprintf(dirName, "strozzij.rooms.%d", pid);
  int Dir = mkdir(dirName,  0755);       //source for directory manipulation: Ben Brewster's class notes
//  printf("Result of mkdir(): %d\n", Dir);


  chdir(dirName);

//  make room files  //
  char fileSuffix[] = "_room";
  char currFile[25], writeBuffer[25];
  int file_descriptor;
  ssize_t nwritten;

  int curr_room, check_connect, connectionNum;
  for (curr_room=1; curr_room<8; curr_room++){                    //curr_room=1 is room 1..no room zero..I was an idiot and programmed it this way

    sprintf(currFile, "%s%s", name_array[names[curr_room-1]], fileSuffix);
    file_descriptor = open(currFile, O_WRONLY | O_CREAT, 0600);

    sprintf(writeBuffer, "ROOM NAME: %s\n", name_array[names[curr_room-1]]);
    nwritten = write(file_descriptor, writeBuffer, strlen(writeBuffer)*sizeof(char));



    //write connections now
    connectionNum=1;                                            //iterates later
    for (check_connect=1;check_connect<8;check_connect++){
      if(connections[curr_room][check_connect]==1){
        sprintf(writeBuffer,"CONNECTION %d: %s\n",connectionNum, name_array[names[check_connect-1]]);
        nwritten = write(file_descriptor, writeBuffer, strlen(writeBuffer)*sizeof(char));
        connectionNum+=1;
      }
    }
    connectionNum=0;      //reset it for next room to check

    if(types[curr_room-1]==0){
      sprintf(writeBuffer,"ROOM TYPE: START_ROOM\n\n");
    }
    else if(types[curr_room-1]==2){
      sprintf(writeBuffer,"ROOM TYPE: END_ROOM\n\n");
    }
    else
      sprintf(writeBuffer,"ROOM TYPE: MID_ROOM\n\n");

    nwritten = write(file_descriptor, writeBuffer, strlen(writeBuffer)*sizeof(char));
    close(file_descriptor);
  }

//  closedir(myDirectory);

}




/********************************************************************************
NO LONGER IN USE
*********************************************************************************/
void DisplayAllInfo(int** connections, int* names, int* types){
  int curr_room, check_connect, connectionNum;
  for (curr_room=1; curr_room<8; curr_room++){                    //curr_room=1 is room 1..no room zero..I was an idiot and programmed it this way
    printf("ROOM NAME: %s\n", name_array[names[curr_room-1]]);    //minus one to compensate for making connections array [8][8] while other arrays were [7]
    connectionNum=1;                                            //iterates later
    for (check_connect=1;check_connect<8;check_connect++){
      if(connections[curr_room][check_connect]==1){
        printf("\tCONNECTION %d: %s\n",connectionNum, name_array[names[check_connect-1]]);
        connectionNum+=1;
      }
    }
    connectionNum=0;      //reset it for next room to check

    if(types[curr_room-1]==0){
      printf("ROOM TYPE: START_ROOM\n\n");
    }
    else if(types[curr_room-1]==2){
      printf("ROOM TYPE: END_ROOM\n\n");
    }
    else
      printf("ROOM TYPE: MID_ROOM\n\n");

  }


}



/********************************************************************************
NO LONGER IN USE
*********************************************************************************/
void testFunc(){
  char file[] = "testfile";
  int file_descriptor;
  file_descriptor = open(file, O_WRONLY);
  if (file_descriptor < 0){
    fprintf(stderr, "Could not open %s\n", file);
    file_descriptor = open(file, O_WRONLY | O_CREAT, 0600);

  }
  close(file_descriptor);


}



/********************************************************************************
* Description: filles types array with the necessary types randomly
* Parameters: none
* Returns: integer array with types
* Pre-Conditions: no array with types
* Post-Conditions: new array with types set
*********************************************************************************/
int* determineTypes(){
  int   startroom, endroom, i;
  int*  types = malloc(7*sizeof(int));
  //choose start and end rooms
  startroom=rand()%7; //chooses between rooms 0-6
  endroom=rand()%7;
  do{
    endroom=rand()%6;
  }while(startroom==endroom);

  //printf("startroom: %d\nendroom: %d\n", startroom, endroom);
  for(i=0;i<7;i++){
    if(i==startroom){               //if current room is startroom, set type to 0
      types[i]=0;
    }
    else if(i==endroom){          //if current room is end room, set type to 2
      types[i]=2;
    }
    else{                         //else MID_ROOM
      types[i]=1;
    }
  }

  /*for(i=0;i<7;i++){
    printf("Room #%d is room type: %d\n", i, types[i]);
  }*/

  return types;
}




/********************************************************************************
* Description: randomly choose names and fill int array with their indexes
* Parameters: none
* Returns: integer array with randomly assigned name array indexes
* Pre-Conditions: none
* Post-Conditions: names assigned
*********************************************************************************/
int* determineNames(){
  int* availableRoomNames = malloc(10*sizeof(int));
  int* chosenNames = malloc(7*sizeof(int));
  int i,j,nameNum, alreadyIn;
  for (i=0; i<10; i++){
    availableRoomNames[i]=i;
  }

  i=0;                                 //which room

  while(1){
    nameNum=rand()%10;              //generate number between 1 and 10

    for(j=0;j<7;j++){                 //loop to go through chosen names array
      if(chosenNames[j] == nameNum){  //if names number is already in chose names array
        alreadyIn=1;                  //bool for already in is made true
      }
    }

    if(alreadyIn==0){                 //if already in bool is 0, then put random number into chosennames array
      chosenNames[i]=nameNum;
      i++;
    }
    alreadyIn=0;

    if(i>6){                          //if done picking names, quit
      break;
    }

  }

  free(availableRoomNames);
  return chosenNames;
}





/********************************************************************************
* Description: does exactly as named, creates and assigns room connections in the form of a binary table randomly
* Parameters: none
* Returns: int** binary table of connections
* Pre-Conditions: no connections set
* Post-Conditions: array of connections set
*********************************************************************************/
int** InitializeConnectionsTable(){

  int** connectArray;
  int i, j;
  connectArray = malloc(8 * sizeof(int*));
  for (i=0; i<8; i++){
    connectArray[i] = malloc(8 * sizeof(int));
  }

  int rando, isConnection;
  for(i=1; i<8;i++){            //room getting connections
    rando = (rand() % 4)+3;     //number of connections for this room

    for(j=1; j<8;j++){          //connections to room
      isConnection = rand()%2;   //will return 1 or 0...yes or no
      if ( isConnection == 1 && rando > 0 && i!=j)  { //if
        connectArray[i][j]=1;
        rando-=1;
      }
      else if (rando == (8-j-1) && i!=j && rando > 0){
        connectArray[i][j]=1;
        rando-=1;
      }
      else{
        connectArray[i][j]=0;
      }

    }
    //  printf("rando in row %d is: %d\n", i, rando);
  }


  // check room continuity
  for(i=1;i<8;i++){
    for(j=1;j<8;j++){
      if (connectArray[i][j]!=connectArray[j][i]){    //if a room connection isn't the true for the connected room, make it true.
        connectArray[i][j]=connectArray[j][i];
        //connectArray[j][i]=connectArray[i][j];
      }
    }
  }



  //now make sure all rooms have 3+ connections
  int addConnectto, newConnection;
  while(isArrayFull(connectArray)!=0){                 //while array isn't full
    addConnectto=isArrayFull(connectArray);            //find room index with less than 3 connections
    newConnection=findConnect(connectArray, addConnectto);          //find
    connectArray[addConnectto][newConnection]=1;
    connectArray[newConnection][addConnectto]=1;
  }


  return connectArray;
}



int findConnect(int** array, int avoid){
  int curr_room, connection, roomAvail, connectionsNum;

    for(curr_room=1;curr_room<8;curr_room++){         //for the 7 rooms
      connectionsNum=0;                               //reset for each room number of connections for each search
      for(connection=1; connection<8; connection++){  //go through curr_rooms Connections
        if(array[curr_room][connection]==1){          //if connection
          connectionsNum+=1;                          //iterate connections counter
        }
      }
      if(connectionsNum<6 && curr_room!=avoid){                           //if a room has less than
        roomAvail=curr_room;                          //room available gets the current room being checked
      }
    }
    return roomAvail;
}



/********************************************************************************
* Description: checks if array is full, if not full returns
* Parameters: the array of connections
* Returns: index where room needs another connection
* Pre-Conditions: array is being made
* Post-Conditions: array is being made
*********************************************************************************/
int isArrayFull(int** array){
  int curr_room, connection, notFullRoom, connectionsNum;
  notFullRoom=0;                                    //room that has less than 3 connections

  for(curr_room=1;curr_room<8;curr_room++){         //for the 7 rooms
    connectionsNum=0;                               //(reset for each room)
    for(connection=1; connection<8; connection++){  //go through curr_rooms Connections
      if(array[curr_room][connection]==1){          //if connection
        connectionsNum+=1;                          //iterate connections counter
      }
    }
    if(connectionsNum<3){
      notFullRoom=curr_room;
    }                               //if connections counter <3

  }


  return notFullRoom;
}





//function not in use anymore
void printConnectsArray(int** Array, int col){
  int i,j;
  if(col==0){
    for(i=1;i<8;i++){
      for(j=1;j<8;j++){
        printf("%d\t", Array[i][j]);

      }
      printf("\n");
    }
  }
  else{
    for(j=1;j<8;j++){
      printf("%d\t", Array[col][j]);

    }

  }

}
