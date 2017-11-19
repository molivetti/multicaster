// server.c
// Matthew Olivetti
// Project 3
// CSCI632 Spring 2014

// This program is based off of newtcpserver.c that is provided with the project instructions
// in order to correctly establish a connection with the client.

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>

#include<pthread.h>
#include<fcntl.h>
#include<unistd.h>

#define SERVER_PORT 5432
#define MAX_DATA 100
#define MAX_PENDING 5
#define MAXNAME 32
#define TABLESIZE 20

struct sockaddr_in my_sin;
struct sockaddr_in clientAddr;

int s, new_s, leaveId;
unsigned int len;
int foundClient;

/**************************************************************/
/* Structure of the registration packet */
/**************************************************************/
struct packet{
	short type;
	char data[MAXNAME];
};

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
/* Structure of the global table */
/**************************************************************/
struct global_table{
	int sockid;
	int req_no;
};

/**************************************************************/
/* Initialization of the global table */
/**************************************************************/	
struct global_table record[TABLESIZE];

struct global_table client_info;

/**************************************************************/
/* Prototype for join_handler method for join_handler thread */
/**************************************************************/
	
void join_handler(struct global_table *rec); 

/**************************************************************/
/* Prototype for multicaster method for multicaster thread */
/**************************************************************/

void multicaster();

/**************************************************************/
/* Prototype for leave method for leave thread */
/**************************************************************/

void leave(int *leaveId);

/**************************************************************/
/* Construction of mutual extension thread */
/* When a thread wants to access the global table, it will lock the mutex variable first */
/* Then it will read/update the global table */
/* Then it will unlock the mutex variable */
/**************************************************************/

pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;

/**************************************************************/
/* Declare array of thread objects */
/**************************************************************/
pthread_t threads[2];
	


/**************************************************************/
/* MAIN */
/**************************************************************/

int main(int argc, char* argv[])
{
	// Instantiate multicaster thread
	pthread_create(&threads[1], NULL, (void *) &multicaster, NULL);
	
	// Set every entry's req_no in global table to 0
	int a;
	for (a=0;a<TABLESIZE;a++){
		record[a].req_no = 0;
	}

	/**************************************************************/
	/* setup passive open */
	/**************************************************************/
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("tcpserver: socket");
		exit(1);
	}

	/**************************************************************/
	/* build address data structure */
	/**************************************************************/
	bzero((char*)&my_sin, sizeof(my_sin));
	my_sin.sin_family = AF_INET;
	my_sin.sin_addr.s_addr = INADDR_ANY;
	my_sin.sin_port = htons(SERVER_PORT);


	if(bind(s,(struct sockaddr *)&my_sin, sizeof(my_sin)) < 0){
		perror("tcpclient: bind");
		exit(1);
	}
	listen(s, MAX_PENDING);
	
	
	/**************************************************************/
	/* Wait for connection, then receive and print text */
	/**************************************************************/
	while(1){
	
		/**************************************************************/
		/* 
		Accept new connections from clients
		new_s is the socket ID for each unique client	
		*/
		/**************************************************************/
		
		if((new_s = accept(s, (struct sockaddr *)&clientAddr, &len)) < 0){
			perror("tcpserver: accept");
			exit(1);
		}
		
		/**************************************************************/
		/* Receive first registration packet or a leave packet from client */
		/**************************************************************/
		struct packet packet_reg;
		
		foundClient = 0;
		
		if(recv(new_s,&packet_reg,sizeof(packet_reg),0) <0)
		{
			printf("\nCould not receive registration packet \n");
			close(s);
			exit(1);
		}
		else {
			// Check the packet by its type, either a registration or leave packet
			if ( ntohs(packet_reg.type) == 121 ){
				printf("\nReceived first registration packet \n");
			}
			else if ( ntohs(packet_reg.type) == 321 ) {
				printf("\nReceived leave packet \n");
				// Found leave packet, set foundClient to 1 to avoid registration process and proceed with leave process
				foundClient = 1;
				leaveId = atoi(packet_reg.data);
				// Instantiate a leave thread and pass it the sockid of the client to be removed from global table
				pthread_create(&threads[2], NULL, (void *) &leave, (void *) &leaveId);
				
			}
			else {
				printf("\nReceived unknown packet \n");			
			}
		}
		/**************************************************************/
		/* Check if socket ID already exists in record table */
		/**************************************************************/
		
		int j = 0;
		for (j=0;j<TABLESIZE;j++){

			if( record[j].sockid == new_s ) {
				
				/**************************************************************/
				/* Set foundClient to 1 to signal that the client is found */
				/**************************************************************/
				foundClient = 1;
				break;
			}	
		}
		/**************************************************************/
		/* If we didn't find client in the record table or if we didn't receive a leave packet, then add the client to the record table */
		/**************************************************************/
		
		if (foundClient == 0){
			int m = 0;
			for (m=0;m<TABLESIZE;m++){
				if(record[m].req_no == 0){
					
					// Lock global table so it can be updated
					pthread_mutex_lock(&my_mutex);
					
					record[m].sockid = new_s;
					
					record[m].req_no = 1;
					
					// Unlock global table
					pthread_mutex_unlock(&my_mutex);
					
					client_info = record[m];
					
					// Instantiate a join handler thread and pass it the sockid and req_no of the client
					pthread_create(&threads[0], NULL, (void *) &join_handler, (void *) &client_info);
					
					// Make the main program wait for the join handler thread to complete
					pthread_join(threads[0], NULL);
					break;
					
				}
			}
		}
		
		/* Set foundClient back to 0 again */
		foundClient = 0;
		
	}
	
}


/**************************************************************/
/* Join handler method for join_handler thread */
/**************************************************************/

void join_handler(struct global_table *rec)
{ 
	// Set newsock to the sockid
	int newsock; 
	struct packet packet_reg; 
	newsock = rec->sockid; 
	
	// Wait for one more registration packets from the client 
	while(1){ 
		if(recv(newsock,&packet_reg,sizeof(packet_reg),0)<0) 
		{ 
			printf("\nCould not receive registration packet\n"); 
			exit(1); 
		} 
		else{

			printf("\nJoin_handler received a registration packet for socketid: %d\n", newsock); 
			

			int z;
			// Update the global table
			for (z=0;z<TABLESIZE;z++){
				
				if( record[z].sockid == newsock ){
					// Lock the global table
					pthread_mutex_lock(&my_mutex);
					
					record[z].req_no++;
				
					// Unlock the global table
					pthread_mutex_unlock(&my_mutex);
					if( record[z].req_no == 3 ){
					
						struct packet packet_reg_confirm;
						char buffer[MAXNAME];
						sprintf(buffer,"%d",newsock);
						strcpy(packet_reg_confirm.data,buffer);
						
						// Send acknowledgement to the client 
						if(send(newsock, &packet_reg_confirm, sizeof(packet_reg_confirm), 0) <0){
							printf("\nRegistration confirm packet send failed\n");
						}
						else{
							printf("\nRegistration confirm packet send SUCCESS\n");
						}
						// Leave the thread 
						pthread_exit(NULL); 
					}
				}
				
			}

			
			
		}
	}
	
} 

/**************************************************************/
/* Multicaster method for multicaster thread */
/**************************************************************/

void multicaster()
{
	char *filename; 
	char text[100]; 
	struct data_packet filedata; 
	int fd;
	int nread;
	int g; 
	int h;
	h = 0;
	
	int newsock; 
	 
	filename = "multicasterinput.txt"; 
	fd = open(filename, O_RDONLY, 0); 
	while(1){
		// Delay transmission for 2 seconds
		sleep(2);
		// Increment header# 
		h++;
		// Read the next 100 characters from the txt file
		nread = read(fd, text ,100);
		
		// Construct the data packet 
		struct data_packet my_data_packet;
		my_data_packet.header = h;
		strcpy(my_data_packet.data, text);
		
		// Check whether any client is listed on the global table 
		for (g=0;g<TABLESIZE;g++){
			
			// If at least one client is listed, send the data packet to the client
			if( record[g].req_no == 3 ){
				
				newsock = record[g].sockid; 
				
				// Send the data packet to the client
				if(send(newsock, &my_data_packet, sizeof(my_data_packet), 0) <0){
					printf("\nDATA packet send failed\n");
				}
				else{
					// Print response sent by server
					printf("%s\n",my_data_packet.data);
				}
				
			}
		}
		fflush(stdout);
	}
}

/**************************************************************/
/* Leave method for leave thread */
/**************************************************************/

void leave(int *leaveId)
{
	// Search through until we find the entry with a matching sockid to leaveId
	int t;
	int sockidTemp;
	for (t=0;t<TABLESIZE;t++){		
		if( record[t].sockid == *leaveId ){
			// Lock the global table, delete the entry, unlock the global table, exit leave thread
			
			sockidTemp = record[t].sockid;
			
			pthread_mutex_lock(&my_mutex);
			
			record[t].sockid = 0;
			record[t].req_no = 0;
			
			pthread_mutex_unlock(&my_mutex);
			
			printf("Sockid:%d is now:%d and thus deleted from the global table\n",sockidTemp, record[t].sockid);
			
			pthread_exit(NULL); 
		}
	}
}

