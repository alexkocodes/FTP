#include<stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<string.h>
#include<pthread.h>
#include <time.h>

#define MAX_SOCKET_NUM 10

struct Users{
	char* username;
	char *password;
	int sock_num;
};


int main()
{

	// creating user profiles
	struct Users user1;

	user1.username = "Alex";
	user1.password = "pass";
	user1.username = 0;



	//1. socket
	int server_fd = socket(AF_INET,SOCK_STREAM,0);
	if(server_fd<0)
	{
		perror("Socket Failed: ");
		return -1;
	}

	//2. bind
	struct sockaddr_in server_address;	//structure to save IP address and port
	memset(&server_address,0,sizeof(server_address)); //Initialize/fill the server_address to 0
	server_address.sin_family = AF_INET;	//address family
	server_address.sin_port = htons(9000);	//port
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); //htonl(INADDR_LOOPBACK); //inet_addr("127.0.0.1");

	setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&(int){1},sizeof(int)); //&(int){1},sizeof(int)


	if(bind(server_fd,(struct sockaddr *)&server_address,sizeof(server_address))<0) 
	{
		perror("Bind failed..:");
		return -1;
	}

	//3. listen
	if(listen(server_fd,5)<0)
	{
		perror("Listen Error:");
		return -1;
	}

	

	struct sockaddr_in client_address;
	int socket_list[MAX_SOCKET_NUM] = {0}; // initiate 10 sockets
	fd_set socks; /* A set of file descriptors we want to use for select() */
	
	//4. accept
	while(1)
	{
	
		FD_ZERO(&socks); //FD_ZERO() clears out the fd_set called socks, so that it doesn't contain any file descriptors.
		FD_SET(server_fd, &socks); //FD_SET() adds the file descriptor "server_fd" to the fd_set, so that select() will return if a connection comes in on that socket (which means you have to do accept(), etc.

		int max_fd = server_fd; // set the initial max bit

		for (int i=0; i<MAX_SOCKET_NUM; i++){	
			if(socket_list[i]>0){ 
				FD_SET(socket_list[i], &socks); // add a new socket to the set
			}
			if(socket_list[i] > max_fd){ // update max socket bit in the set
				max_fd = socket_list[i];
			}
		}
		

		if (select(max_fd + 1, &socks, NULL, NULL, NULL) < 0) // select function will block until something happens on one of the sockets in the set
		{
			perror("Select failed.\n");
			return -1;
		}

		// loop through the set to check each socket
		for (int i=0; i<max_fd; i++){
			if(FD_ISSET(i, &socks)){
				if(i == server_fd){ // if it's the server socket chosen, it means that there's a new connection coming
					int client_sd = accept(server_fd,NULL,NULL);
					if(client_sd<1)
					{
						perror("Accept Error:");
						return -1;
					}
					FD_SET(client_sd, &socks);
					if(client_sd> max_fd){ // update max socket bit in the set
						max_fd = socket_list[i];
					}

					// find the first 0 and set it to client_sd
					for(int i=0; i<MAX_SOCKET_NUM; i++){
						if(socket_list[i]==0){
							socket_list[i] = client_sd;
						}
						printf("New Connection established!");
						break;
					}
					break;
				}
				else{ // if it's not server, start serving files
					
				}
			}
		}
	}	

	close(server_fd);										//close the master/server socket
	//6. close
	return 0;
}