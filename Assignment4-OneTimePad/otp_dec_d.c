/*
AUTHOR: Joshua Strozzi
PROGRAM: otp decoder DAEMON
CLASS: CS-344
DUE: NOV 11 - 11:59PM
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

//look for comments in otp_enc_d

pid_t myPids[5] = {-5,-5,-5,-5,-5};                             //set all values of pid array to -5, if -5, then put new pid there
int numPids = 0;                                    //curr number of pids....starts at 0
int globalExitStat=-5;

int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char buffer[1000];
	struct sockaddr_in serverAddress, clientAddress;

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	// Accept a connection, blocking if one is not available until one connects
	sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
  int it;  //for loop iterator
//while loop
  while(1){
    //start by going through array of pids and checking if any need reaping, but there will always be one left over
    if(numPids>0){                              //if num of active pids is more than 0
      for(it=0; it<5; it++){                    //go through pid array
        if(myPids[it] > 0){                     //if curr pid is not -5, then attempt to reap it
          int currPid = myPids[it];
          if(waitpid(myPids[it], &globalExitStat, WNOHANG)){
            myPids[it]=-5; //reset curr pid array to -5 for later used
            numPids -= 1;
            //printf("Background pid %d reaped, totalpids: %d\n", currPid, numPids);
            fflush(stdout);
          }
        }
      }
    }



    pid_t spawnpid = -5;
   	//If there are already 5 connections, then wait, else don't wait, go to accept
    establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
  	if (establishedConnectionFD < 0) error("ERROR on accept");

    char *mainBuff, *msg, *key;                                     //the main buffer

    //if no accept error, then fork...in child, do processing
    spawnpid = fork();

    switch(spawnpid){
      case -1:
        fprintf(stderr, "HULL BREACH!\n");
        exit(1);
      break;
      case 0:     //child process
        // Get the message from the client and put it in buffer
        mainBuff= malloc(sizeof(char) * 150000);        //main buffer
        memset(mainBuff, '\0', sizeof(mainBuff));       //fill with nulls

        int totalread=0;                                //keep track of total read in
        while(buffer[strlen(buffer)-1]!='$'){
        	memset(buffer, '\0', 1000);
        	charsRead = recv(establishedConnectionFD, buffer, 1000, 0); // Read the client's message from the socket
          totalread+=charsRead;
          if (charsRead < 0) error("ERROR reading from socket");
          //printf("SERVER: I received this from the client: \"%s\"\n", buffer);
          strncat(mainBuff, buffer, 1000);                            //past all stuff in buffer into mainBuff[er]
        }

        //check if meant for encryptor or decryptor ....turns out this is the job of the client
        if(mainBuff[0]=='e'){
        //  fprintf(stderr,"ERROR: otp_enc cannot use otp_dec_d\n");
          	fflush(stdout);
		charsRead = send(establishedConnectionFD, "1", 1, 0);

					//exit(2);
        }
        //printf("full message: %s", mainBuff);
        char *startptr, *endptr;                       //set points to char array
        //separate message from client string

        startptr = strstr(mainBuff, "#");
        startptr++;
        endptr = strstr(mainBuff, "@");
        msg =malloc( ( strlen(startptr)-strlen(endptr) ) *sizeof(char));
        memset(msg, '\0', sizeof(msg));
        strncpy(msg, startptr, strlen(startptr)-strlen(endptr));
        //printf("msg: %s\n", msg);

        //separate key from strings

        startptr = endptr;																								//start from @ symbol
        startptr++;																												//increment it 1 index to the first letter in the key
        endptr = strstr(mainBuff, "$");																		//set end pointer to terminating symbol
        key =malloc( ( strlen(startptr)-strlen(endptr) ) *sizeof(char));	//mallocate key string to size of the difference of the rest of the string and the @ symbol string portion
        memset(key, '\0', sizeof(key));																		//make sure it's empty
        strncpy(key, startptr, strlen(startptr)-strlen(endptr));					//copy string into key
        //printf("key: %s\n", key);

        free(mainBuff);

        //NOW DECRYPT AND SEND IT BACK
        char* code = malloc(sizeof(char) * (strlen(msg)+1) );
        char *keyptr, *msgptr, *codeptr;
        int i, currchar;

	int c, k; 				//ints for cypher char and key char
        for(i=0, keyptr=key, msgptr=msg, codeptr=code; i<strlen(msg); i++, msgptr++, codeptr++, keyptr++){

		if(*msgptr ==' '){				//if space
			c=26;										//make it a bracket
		}
		else{
			c= *msgptr - 'A';				//else convert it to it's letter number
		}
		if(*keyptr == ' '){				//repeat
			k= 26;
		}
		else{
			k = *keyptr - 'A';
		}

		currchar = c-k;			// = cypher - key

		if(currchar<0){			//if less that A
			currchar+=27;			//add 27 to make it it's letter
		}

		if(currchar==26){	//if bracket
			*codeptr=' ';		//make it a space
		}
		else{
			*codeptr=currchar +'A';		//else make it it's letter
		}

        }
        code[strlen(msg)]='$';		//append the terminating character onto the end of it

        free(msg);
        free(key);


        // Send a Success message back to the client
      	//charsRead = send(establishedConnectionFD, "I am the server, and I got your message", 39, 0); // convert to returning encrypted message
        int totalsent=0;
        char* outptr = code;
        while(totalsent<strlen(code)){																																				//while total sent is less that total to send
          charsRead=send(establishedConnectionFD, outptr+totalsent*sizeof(char), strlen(code)-totalsent, 0); // Write to the client	as much as possible starting from end of last portion sent
          totalsent+=charsRead;
          if (charsRead < 0) error("CLIENT: ERROR writing to socket");
          if (charsRead < strlen(code)) printf("CLIENT: WARNING: Not all data written to socket!\n");
        }


				//free(code);
        exit(0); //to prevent forkbombs
      break;
      default:
        //parent process
      break;
    }

    //put pid into array of myPids
    for(it =0; it< 5; it++){
      if( myPids[it]==-5){
        myPids[it]=spawnpid;     //add to pid array at next instance of -5 in array
        numPids +=1;              //add 1 to num of pids in array
        break;
      }
    }
  }
  //end while loop?
	if (charsRead < 0) error("ERROR writing to socket");
	close(establishedConnectionFD); // Close the existing socket which is connected to the client
	close(listenSocketFD); // Close the listening socket
	return 0;
}
