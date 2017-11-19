// client.c
// Matthew Olivetti
// Project 3 
// CSCI632 Spring 2014

// This program is based off of newtcpclient.c that is provided with the project instructions
// in order to correctly establish a connection with the server.

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h> 
#include<netinet/in.h>
#include<netdb.h>
 
#define SERVER_PORT 5432
#define MAX_LINE 256
#define MAXNAME 32
#define MAX_DATA 100

int main(int argc, char* argv[])
{
	
	struct hostent *hp;
	struct sockaddr_in sin;
	char *host;
	char buf[MAX_LINE];
	int s;
	int len;
	int packNum;
	int packCheck;
	char clientSockid[MAXNAME];

	/**************************************************************/
	/* structure of the registration packet */
	/**************************************************************/
	struct packet{
		short type;
		char data[MAXNAME];
	};

	struct packet packet_reg;
	
	/**************************************************************/
	/* 
		Structure of the data_packet packet 
		header is the packet# that the multicaster increments
	*/
	/**************************************************************/
	struct data_packet{
		short header;
		char data[MAX_DATA];
	};
	
	
	/**************************************************************/
	/* Constructing the registration packet at client */
	/**************************************************************/
	packet_reg.type = htons(121);
	char name[MAX_LINE];
	gethostname(name, MAX_LINE); 
	
	strcpy(packet_reg.data, name);

	if(argc == 4){
		host = argv[1];
		packNum = atoi(argv[3]);
	} 
	else{
		fprintf(stderr, "usage:newclient server\n");
		exit(1);
	}

	/* translate host name into peer's IP address */
	hp = gethostbyname(host);
	if(!hp){
		fprintf(stderr, "unkown host: %s\n", host);
		exit(1);
	}

	/* active open */
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("tcpclient: socket");
		exit(1);
	}
	
	/**************************************************************/
	/* build address data structure */
	/**************************************************************/
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);

	
	if(connect(s,(struct sockaddr *)&sin, sizeof(sin)) < 0){
		perror("tcpclient: connect");
		close(s);
		exit(1);
	}
	
	/**************************************************************/
	/* Send out three registration request packets to server */
	/**************************************************************/
	if(send(s, &packet_reg, sizeof(packet_reg), 0) <0)
	{	
		printf("\nFirst registration packet send failed\n");
	}
	else{
		printf("\nSent first registration packet \n");
	}

	if(send(s, &packet_reg, sizeof(packet_reg), 0) <0)
	{	
		printf("\nSecond registration packet send failed\n");
	}
	else{
		printf("\nSent second registration packet \n");
	}
	
	if(send(s, &packet_reg, sizeof(packet_reg), 0) <0)
	{	
		printf("\nThird registration packet send failed\n");
	}
	else{
		printf("\nSent third registration packet \n");
	}

	struct packet packet_reg_confirm;
	struct data_packet my_data_packet;
	
	/**************************************************************/
	/* Receive packet that confirms registration from server*/
	/**************************************************************/
	
	if(recv(s, &packet_reg_confirm, sizeof(packet_reg_confirm), 0) <0)
	{
		printf("\nCould not receive registration confirm packet \n");
		
	}
	else{
		printf("\nReceived registration confirmation packet \n\n");
		// Copy registration packet's data to clientSockid, which is the sockid used to identify the client in the global table
		strcpy(clientSockid, packet_reg_confirm.data);
		// Set packCheck to 0 to keep track of how many packets the client has received
		packCheck = 0;
		while(1){
			
			if(recv(s, &my_data_packet, sizeof(my_data_packet), 0) <0){
				printf("\nCould not receive data packet \n");
				close(s);
				exit(1);
			}
			else{
				
				// Print the received text from the multicaster
				printf("%s\n",my_data_packet.data);
				packCheck = packCheck + 1;
				
				if ( packCheck == packNum ) {
					
					// Construct leave packet, set its type to 321 and give it the sockid to be deleted from global table
					struct packet leave;
					leave.type = htons(321);
					strcpy(leave.data, clientSockid);
					
					
					// Create new connection to server in order to send leave packet to server
					/* translate host name into peer's IP address */
					hp = gethostbyname(host);
					if(!hp){
						fprintf(stderr, "unkown host: %s\n", host);
						exit(1);
					}

					/* active open */
					if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
						perror("tcpclient: socket");
						exit(1);
					}
					
					/**************************************************************/
					/* build address data structure */
					/**************************************************************/
					bzero((char*)&sin, sizeof(sin));
					sin.sin_family = AF_INET;
					bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
					sin.sin_port = htons(SERVER_PORT);
					
					// Create new connection to server
					if(connect(s,(struct sockaddr *)&sin, sizeof(sin)) < 0){
						perror("tcpclient: connect");
						close(s);
						exit(1);
					}
					
					// Send leave packet to server
					if(send(s, &leave, sizeof(leave), 0) <0)
					{	
						printf("\n Leave packet send failed\n");
					}
					else{
						printf("\n Sent leave packet, client program has exited\n");
						exit(1);
					}
				}
			}
			
	
		}
	}
}
