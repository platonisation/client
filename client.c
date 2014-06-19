#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>           /*  misc. UNIX functions      */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/*  Global constants  */

#define MAX_LINE           (1000)

#include "dependencies/communication/communication.h"

void sendUserDatas(int sock,char* username);
void createFile(char* msg);

/*  main()  */

int main(int argc, char *argv[]) {

	int quit = 0;

    int       conn_s;                /*  connection socket         */
    short int port;                  /*  port number               */
    struct    sockaddr_in servaddr;  /*  socket address structure  */
    char      buffer[MAX_LINE];      /*  character buffer          */
    char     *szAddress;             /*  Holds remote IP address   */
    char     *szPort;                /*  Holds remote port         */
    char     *endptr;                /*  for strtol()              */
    fd_set read_selector;
    char* msgToSend = malloc(sizeof(char)*MAX_LINE);
    char username[MAX_USR_LENGTH];
    int ret;


    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    /*  Get command line arguments  */

    ParseCmdLine(argc, argv, &szAddress, &szPort, username);


    /*  Set the remote port  */

    port = strtol(szPort, &endptr, 0);
    if ( *endptr ) {
		printf("ECHOCLNT: Invalid port supplied.\n");
		exit(EXIT_FAILURE);
    }


    /*  Create the listening socket  */

    if ( (conn_s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		fprintf(stderr, "ECHOCLNT: Error creating listening socket.\n");
		exit(EXIT_FAILURE);
    }


    /*  Set all bytes in socket address structure to
        zero, and fill in the relevant data members   */

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_port        = htons(port);


    /*  Set the remote IP address  */

    if ( inet_aton(szAddress, &servaddr.sin_addr) <= 0 ) {
		printf("ECHOCLNT: Invalid remote IP address.\n");
		exit(EXIT_FAILURE);
    }


    /*  connect() to the remote echo server  */

    if ( connect(conn_s, (struct sockaddr *) &servaddr, sizeof(servaddr) ) < 0 ) {
	printf("ECHOCLNT: Error calling connect()\n");
	exit(EXIT_FAILURE);
    }



    sendUserDatas(conn_s, username);

    printf("Entrez vos commandes\n");

    while(!quit){

        FD_ZERO(&read_selector);
       	 /* Remember the main socket */
        FD_SET(STDIN_FILENO,&read_selector);
        FD_SET(conn_s,&read_selector);

        ret = select(FD_SETSIZE,&read_selector,(fd_set *)NULL,(fd_set *)NULL,&tv);
        if(ret>0){
        	if(FD_ISSET(STDIN_FILENO, &read_selector)) {
        		if (!fgets(buffer, sizeof(buffer), stdin)){
        			if (ferror(stdin)) {
						perror("cannot read the message you wrote\n");
						exit(1);
					}
        		}
				if(strcmp(buffer,"quit\n") == 0){
					quit=1;
				}
				else {
					/*  Send string to echo server, and retrieve response  */
					msgToSend = parseMessage(buffer,strlen(buffer));
					Writeline(conn_s, msgToSend, strlen(buffer)+1);
				}
        	}
        	else if(FD_ISSET(conn_s, &read_selector)){
        		if(Readline(conn_s, msgToSend, MAX_LINE-1) > 0){
        			if(strstr(msgToSend,"ENDOFFILE")) {
						char* file = malloc(strlen(msgToSend)*sizeof(char));
						char* temp = malloc(strlen(msgToSend)*sizeof(char));
						strcpy(temp,msgToSend);
						file = strtok(msgToSend,";");
						if(file != NULL && strcmp(file,"ENDOFFILE")){ //THIS IS A FILE
							temp = strtok(NULL,"\0");
							temp[strlen(temp)]=0;
							printf("A : %s\n",temp);
							debugTrace("Received a file");
							createFile(temp);

						}
        			}
					else{
						printf("%s\n",msgToSend);
					}
				}
        		else
        			printf("Read error\n");
        	}
        	else {
        		printf("EXIT\n");
        		exit(0);
        	}
        }
        else if(ret == 0){
//        	exit(0);
        }
        sleep(1);
    }

		/*  Close the connected socket  */

	if ( close(conn_s) < 0 ) {
		fprintf(stderr, "ECHOSERV: Error calling close()\n");
		exit(EXIT_FAILURE);
		debugTrace("LE CLIENT IS DEAD !\n");
	}

    return EXIT_SUCCESS;
}


int ParseCmdLine(int argc, char *argv[], char **szAddress, char **szPort, char * username) {

    int n = 1;

    while ( n < argc ) {
	if ( !strncmp(argv[n], "-a", 2) || !strncmp(argv[n], "-A", 2) ) {
	    *szAddress = argv[++n];
	}
	else if( !strncmp(argv[n], "-u", 2) || !strncmp(argv[n], "-U", 2) ) {
		strncpy(username,argv[++n],sizeof(username));
		username[strlen(username)] = '\0';
		printf("Bienvenue %s\n",username);
	}
	else if ( !strncmp(argv[n], "-p", 2) || !strncmp(argv[n], "-P", 2) ) {
	    *szPort = argv[++n];
	}
	else if ( !strncmp(argv[n], "-h", 2) || !strncmp(argv[n], "-H", 2) ) {
	    printf("Usage:\n\n");
	    printf("    timeclnt -a (remote IP) -p (remote port)\n\n");
	    exit(EXIT_SUCCESS);
	}
	else if ( !strncmp(argv[n], "-d", 2) || !strncmp(argv[n], "-D", 2) ) {

	}
	++n;
    }

    return 0;
}

void createFile(char* msg){
	printf("TS%s\n",msg);
	FILE* file = NULL;
	file = fopen("receive","w+");
	if(file != NULL){
		fputs(msg, file);
	}
}

void sendUserDatas(int sock,char* username) {
	char* buf = malloc(sizeof(char) * (MAX_USR_LENGTH + 4));
	strcat(buf,username);
	strcat(buf," 1 1");
	Writeline(sock,buf,strlen(buf)+1);
}
