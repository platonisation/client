/*
 * client.c
 *
 *  Created on: Apr 16, 2014
 *  Author: Timoté Bonnin
 *
 *  School project :
 *
 *
Dans la continuité des TPs, le pro jet a p our ob jectif la mise en œuvre d'une application de chat client/serveur en C:
**échanger des messages entre 2, plusieurs, ou la totalité des clients connectés sur le réseau ;
**s'envoyer des fichiers ;
**échanger des messages entre clients du même réseau local sans intervention d'un serveur.

Afin de pallier la faiblesse inhérente a une architecture centralisée, votre réseau de chat doit prévoir
la collab oration de plusieurs serveurs afin d'en ourir une vision globale aux utilisateurs connectés à
chaque serveur.


 Client
**Chaque client est identié par un pseudo.
**Un client peut lister les pseudos des utilisateurs connectés au réseau, et ignorer les messages et
demandes d'envoi de chiers d'un ou plusieurs d'entre eux.
**Des group es peuvent être créés par n'imp orte quel utilisateur et n'imp orte quel utilisateur p eut
rejoindre un group e.
**Un utilisateur peut lister les pseudos des utilisateurs d'un group e. Un group e représente un sous-
ensemble d'utilisateurs. Les messages émis à destination d'un group e sont transmis à l'ensemble
des utilisateurs appartenant à ce group e.
**L'utilisateur qui a créé le group e a le droit d'expulser les autres utilisateurs qui ont rejoint le
groupe

Principe le client envoie une commande au serveur, le serveur répond. exemple :

> ??
< liste des commande : ...
______________________

> help
<liste des commande : ...
 *
 */


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

    printf("Entrez vos commande\n");

    while(!quit){

        FD_ZERO(&read_selector);
    //    	Update FD's : En sortie, les ensembles sont modifiés pour  indiquer  les
    //    	descripteurs qui ont changé de statut.
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
//					printf("%s\n", msgToSend);
        			char* sv;
        			char* file=NULL;
					file = strtok_r(msgToSend,";",&sv);
					if(file != NULL && strcmp(sv,"")){ //THIS IS A FILE
						printf("THIS IS A FILE%s\n",sv);
						createFile(sv);

					}
				}
        		else
        			printf("Jai pas réussi a lire\n");
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
//		printf("argv(lebon) : %s, size de username  = %d\n",argv[n+1], strlen(*username));
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
	FILE* file = NULL;
	file = fopen("receive","w+");
	printf("BITE\n");
	if(file != NULL){
		fputs(msg, file);
//		int c = fclose(a);
	}
	printf("REFINI\n");
}

void sendUserDatas(int sock,char* username) {
	char* buf = malloc(sizeof(char) * (MAX_USR_LENGTH + 4));
	strcat(buf,username);
	//FIXEME:APASBIEN
	strcat(buf," 1 1");
	Writeline(sock,buf,strlen(buf)+1);
}


///*  Read a line from a socket  */
//
//int Readline(int sockd, char* buffer, size_t maxlen) {
//
//	//data to read
//	unsigned char start;
//	unsigned char src[4];
//	unsigned char dst[4];
//	unsigned char size;
//	unsigned char data[20];
//
////	if ( (rc = read(sockd, data, 12)) == 12 ){
////	printf("\n\nCOCOU\n\n");
////	printf("Val : %s\n",data);
////	}
//
//	//read selection
//	fd_set read_selector;
//	//timeout of read
//	struct timeval timeout;
//	timeout.tv_sec = 30;
//	timeout.tv_usec = 0;
//	//return value
//	int retval;
//	//init read selection
//	FD_ZERO(&read_selector);
//	FD_SET(sockd,&read_selector);
//	retval = select(sockd+1,&read_selector,NULL,NULL,NULL);
//	if(retval) {
//		//treat data
//		read(sockd, &start, 1);
//		if (start == 0xFE) {
////			read(sockd,src,4);
////			read(sockd,dst,4);
//			read(sockd,&size,1);
//			//size in bytes
//			if(size < 10000000){//10Mo, msg
//				if((read(sockd,buffer,size) != size)){
//					//Lecture ok
//					printf("Cannot read %d datas\n",size);
//				}
//				else {
////					doAction(buffer,size);
//				}
//
//			}
//			else { // files
//				//attention bug ! client envoie coucou trouve un fichier
//				printf("This is a file\n");
//			}
//		}
//		else {
//			printf("Not a starting sequence\n");
//		}
//	}
//	else if(retval == -1){
//		//treat error
//		printf("RETVAL==1\n");
//	}
//	else{
//		//treat no data found
//		printf("NODATA?\n");
//	}
//
//	return 1;
//}
//
//
///*  Write a line to a socket  */
//
//ssize_t Writeline(int sockd, const void *vptr, size_t n) {
//
//	size_t      nleft;
//    ssize_t     nwritten;
//    char* buffer;//attention a ne pas depasser
//    buffer = vptr;
//    nleft  = n;
//
////   unsigned char buffer[12]={0xFE,0x01,0x69};
////    if(write(sockd, buffer, 3) <0){
////    	printf("FAILIED");
////    }
//
//    while ( nleft > 0 ) {
//	if ( (nwritten = write(sockd, buffer, nleft)) <= 0 ) {
//	    if ( errno == EINTR )
//		nwritten = 0;
//	    else
//		return -1;
//	}
//	nleft  -= nwritten;
//	buffer += nwritten;
//    }
//
//    return n;
//}

//char* parseMessage(char* buffer, int size) {
//	char* s;
//	s = malloc(size*sizeof(char) + 2);
//	s[0]=0xFE;
//	s[1]=strlen(buffer);
//
//	strcat(s,buffer);
//
//	return s;
//}


