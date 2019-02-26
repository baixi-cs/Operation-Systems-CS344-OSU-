/*
AUTHOR: Joshua Strozzi
PROGRAM: otp decoder CLIENT
CLASS: CS-344
DUE: NOV 11 - 11:59PM
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues


int main(int argv, char *argc[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	char* inputfile;							//cypher
	char* keyfile;								//key
	inputfile = argc[1];
	keyfile = argc[2];



	//get files
	FILE * fd1, *fd2;														//my file pointers

	fd1 = fopen(inputfile, "r");								//open input file
	if(!fd1){
		fprintf(stderr,"fd1 fopen() was an utter failure\n");
		fflush(stdout);
		exit(1);
	}

	fd2 = fopen(keyfile, "r");									//open keyfile
  if(!fd2){
    fprintf(stderr, "fd2 open() was an utter failure");
    fflush(stdout);
	  exit(1);
  }

	fseek(fd1, 0, SEEK_END);						//go to end of file
	int inputlen = ftell(fd1);					//get file length
	fseek(fd2, 0, SEEK_END);						//repeat for key file
  int keylen = ftell(fd2);

	if(inputlen > keylen){							//compare size of input to size of key
    fprintf(stderr, "Key isn't big enough for the input file\n");
    fflush(stdout);
    exit(1);
  }
  //reset file pointer to beginning
	fseek(fd1, 0, SEEK_SET);
  fseek(fd2, 0, SEEK_SET);

	char* inbuff = malloc(inputlen*sizeof(char));			//set up cypher array
	memset(inbuff, '\0', inputlen*sizeof(char));			//clear it

	if(fgets(inbuff, inputlen, fd1)!=NULL){						//fill cypher array with the stuff

		inbuff[inputlen-1]='\0';
	}
	fclose(fd1);


	char* keybuff = malloc(keylen*sizeof(char));			//repeat for the key
  if(fgets(keybuff, keylen, fd2)!=NULL){

    keybuff[keylen-1]='\0';													//instead of last character being a newline, it's a null terminator
  }
  fclose(fd2);

	//now concat those and send it to the servers
	int size =  (sizeof(char)*inputlen) + ( sizeof(char) * keylen) + (5*sizeof(char)) ;
	char* output = malloc(size);
	memset(output, '\0', size );

//	printf("sizeof array: %d\n", size);
	sprintf(output, "d#%s@%s$", inbuff, keybuff);				//cat together my symbols, message, and key
	//set up output to server

	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[256];				//don't use

	if (argv < 4) { fprintf(stderr,"USAGE: %s hostname port\n", argc[0]); exit(0); } // Check usage & args

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argc[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address					//instead of using a cmd arg for "localhost", just set it by name
	//serverHostInfo = gethostbyname(argc[1]); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");


	int totalsent=0, lastsent =0;
	char* outptr = output;
	while (totalsent<strlen(output) ){																														//while total sent is less that total characters
		charsWritten = send(socketFD, outptr+totalsent*sizeof(char), strlen(output)-totalsent, 0); // Write to the server	as much as possible starting from end of last portion sent
		totalsent+=charsWritten;																																	//add last sent to total sent
		if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
		if (charsWritten < strlen(output)) printf("CLIENT: WARNING: Not all data written to socket!\n");
	}




	char* deciphbuff= malloc(sizeof(char)* 70001);
	//char* readbuff = malloc(sizeof(char)*1000);
	char readbuff[1000];
	memset(deciphbuff, '\0', sizeof(deciphbuff));
	memset(readbuff, '\0', sizeof(readbuff));
	while(readbuff[strlen(readbuff)-1] !='$'){															//while terminating character not found, keep reading
		memset(readbuff, '\0', 1000);																					//make sure buffer is empty
		charsWritten = recv(socketFD, readbuff, 1000, 0);											//take in 1000 chars at a time and keep track of chars read
		if(charsWritten < 0) error("ERROR reading from server socket");				//
		if(readbuff[0]=='1'){																									//if error message received from daemon, quit
			fprintf(stderr,"ERROR: otp_dec cannot use otp_enc_d\n");
			fflush(stdout);
			exit (2);
		}

		strncat(deciphbuff, readbuff, 1000);																	//glue read buff onto the decypher buffer

	}

	deciphbuff[strlen(deciphbuff)-1]='\n';																	//remove terminating character and replace with newline

	printf("%s", deciphbuff);




	//free(readbuff);
	free(deciphbuff);
	close(socketFD); // Close the socket
	return 0;
}
