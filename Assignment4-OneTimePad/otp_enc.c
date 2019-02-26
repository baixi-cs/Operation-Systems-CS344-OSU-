/*
AUTHOR: Joshua Strozzi
PROGRAM: otp encoder DAEMON
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
	char* inputfile;
	char* keyfile;
	inputfile = argc[1];
	keyfile = argc[2];

	//printf("input: %s | keyfile: %s\n", inputfile, keyfile);

	//get files
	FILE * fd1, *fd2;														//my file pointers

	fd1 = fopen(inputfile, "r");								//open input file
	if(!fd1){
		fprintf(stderr, "fd1 fopen() was an utter failure\n");
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

	char* inbuff = malloc(inputlen*sizeof(char));			//set up text array
	memset(inbuff, '\0', inputlen*sizeof(char));			//clear it

	if(fgets(inbuff, inputlen, fd1)!=NULL){						//fill array with the stuff

		inbuff[inputlen-1]='\0';
	}
	fclose(fd1);


	char* keybuff = malloc(keylen*sizeof(char));			//repeat for the key
  if(fgets(keybuff, keylen, fd2)!=NULL){

    keybuff[keylen-1]='\0';													//instead of last character being a newline, it's a null terminator
  }
  fclose(fd2);
	//printf("input: %s", inbuff);
	//now concat those and send it to the servers
	int size =  (sizeof(char)*inputlen) + ( sizeof(char) * keylen) + (5*sizeof(char)) ;
	char* output = malloc(size);
	memset(output, '\0', size );

//	printf("sizeof array: %d\n", size);
	sprintf(output, "e#%s@%s$", inbuff, keybuff);				//cat together my symbols, message, and key
	//set up output to server
	//printf("catted: %s\n",output);
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[256];

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
	//printf("CLIENT: pre sending\n");
	while (totalsent<strlen(output) ){
		charsWritten = send(socketFD, outptr+totalsent*sizeof(char), strlen(output)-totalsent, 0); // Write to the server	as much as possible starting from end of last portion sent
		totalsent+=charsWritten;
		if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
		if (charsWritten < strlen(output)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	}

	//printf("CLIENT chars sent: %d\n", totalsent);

	// Get return message from server

	int totalread=0;
	char* ciphbuff= malloc(sizeof(char)* 70001);
	//char* readbuff = malloc(sizeof(char)*1000);
	char readbuff[1000];
	memset(ciphbuff, '\0', sizeof(ciphbuff));
	memset(readbuff, '\0', sizeof(readbuff));
	while(readbuff[strlen(readbuff)-1] !='$'){														//while terminating character not found, keep reading
		//memset(readbuff, '\0', sizeof(readbuff));
		memset(readbuff, '\0', 1000);																				//clear up the array
		charsWritten = recv(socketFD, readbuff, 1000, 0);										//send 1000 characters at a time
		totalread +=charsWritten;																						//keep trck of total read
		if(charsWritten < 0) error("ERROR reading from server socket");
		if(readbuff[0]=='1'){
			fprintf(stderr, "ERROR: otp_enc cannot use otp_dec_d\n");
			fflush(stdout);
			exit (2);
		}
		strncat(ciphbuff, readbuff, 1000);																	//glue together the cipher as it comes in
		//printf("1");
	}
	//printf("CLIENT: chars recieved: %d\n", totalread);
	ciphbuff[strlen(ciphbuff)-1]=='\0';																		//remove terminating character and replace it with newline

	printf("%s", ciphbuff);




	//free(readbuff);
	free(ciphbuff);
	close(socketFD); // Close the socket
	return 0;
}
