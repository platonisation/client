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

 Serveur
**Des notications sont envoyées aux clients à chaque connexion et déconnexion.
**Chaque serveur doit maintenir une liste des membres connectés sur le réseau.
**Un utilisateur p eut créer un compte (pseudo / mot de passe). Ce doit être une notion globale au
réseau : un compte enregistré depuis un serveur doit être reconnu à une pro chaine connexion sur
un autre serveur.
**Un utilisateur enregistré p eut être administrateur du réseau. Il p eut alors expulser bannir un
autre utilisateur



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


/*  Function declarations  */

int ParseCmdLine(int argc, char *argv[], char **szAddress, char **szPort);

/*  Read a line from a socket  */

ssize_t Readline(int sockd, void *vptr, size_t maxlen) {
    ssize_t n, rc;
    char    c, *buffer;

    buffer = vptr;

    for ( n = 1; n < maxlen; n++ ) {

	if ( (rc = read(sockd, &c, 1)) == 1 ) {
	    *buffer++ = c;
	    if ( c == '\n' )
		break;
	}
	else if ( rc == 0 ) {
	    if ( n == 1 )
		return 0;
	    else
		break;
	}
	else {
	    if ( errno == EINTR )
		continue;
	    return -1;
	}
    }

    *buffer = 0;
    return n;
}


/*  Write a line to a socket  */

ssize_t Writeline(int sockd, const void *vptr, size_t n) {

	size_t      nleft;
    ssize_t     nwritten;
//     char *buffer;

//    buffer = vptr;
    nleft  = n;

   unsigned char buffer[12]={0xFE,0x01,0x69};
    if(write(sockd, buffer, 3) <0){
    	printf("FAILIED");
    }

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

    return n;
}



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


    /*  Get command line arguments  */

    ParseCmdLine(argc, argv, &szAddress, &szPort);


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

    while(!quit){

		/*  Get string to echo from user  */

		printf("Enter the string to echo: ");
		fgets(buffer, MAX_LINE, stdin);

		if(strcmp(buffer,"quit\n") == 0){
			quit=1;
		}
		else {
			/*  Send string to echo server, and retrieve response  */

			Writeline(conn_s, buffer, strlen(buffer));
			Readline(conn_s, buffer, MAX_LINE-1);


			/*  Output echoed string  */

			printf("Echo response: %s\n", buffer);
		}
    }

    return EXIT_SUCCESS;
}


int ParseCmdLine(int argc, char *argv[], char **szAddress, char **szPort) {

    int n = 1;

    while ( n < argc ) {
	if ( !strncmp(argv[n], "-a", 2) || !strncmp(argv[n], "-A", 2) ) {
	    *szAddress = argv[++n];
	}
	else if ( !strncmp(argv[n], "-p", 2) || !strncmp(argv[n], "-P", 2) ) {
	    *szPort = argv[++n];
	}
	else if ( !strncmp(argv[n], "-h", 2) || !strncmp(argv[n], "-H", 2) ) {
	    printf("Usage:\n\n");
	    printf("    timeclnt -a (remote IP) -p (remote port)\n\n");
	    exit(EXIT_SUCCESS);
	}
	++n;
    }

    return 0;
}

