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
#define MAX_USERS 10

typedef struct User{
	char *username;
	char *password;
	int sock_num;
	int login_status;
} User;


int main()
{

	// read from a txt file
	char *filename = "userinfo.txt";
    FILE *fp = fopen(filename, "r");

    if (fp == NULL)
    {
        printf("Error: could not open file %s", filename);
        return 1;
    }

    // reading line by line, max 256 bytes
    char buffer[256];
	
	User* all_users = malloc(MAX_USERS * sizeof(*all_users));
	int counter = 0;

    while (fgets(buffer, 256, fp)){
		buffer[strcspn(buffer, "\n")] = 0;
		char * token = strtok(buffer, " ");
		int i = 0;

		// all_users[counter].username = malloc(256);
		// all_users[counter].password = malloc(256);

		// loop through this line to extract all other tokens
		while( token != NULL && strcmp(token, "\n") !=0 ) {
			// printf( " %lu\n", sizeof(&token) ); //printing each token
			if(i == 0){
				all_users[counter].username = malloc(128); // allocate memory for username
				strcpy(all_users[counter].username, token);
			}
			else if(i == 1){
				all_users[counter].password = malloc(128); // allocate memory for password
				strcpy(all_users[counter].password, token);	
			}
			token = strtok(NULL, " ");
			i += 1;
			
		}
		all_users[counter].sock_num = 0;
		all_users[counter].login_status = 0;
		counter += 1;
   	}

	for(int i=0; i<counter; i++){
		printf( "Username: %s\n", all_users[i].username ); //printing each user's username
		printf( "Password: %s\n", all_users[i].password ); //printing each user's password
	}
	// when to free them?



    // close the file
    fclose(fp);


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

	close(server_fd);	

	//free the memory
	for(int i=0; i<counter; i++){
		free(all_users[i].username ); 
		free(all_users[i].password );
	}

	return 0;
}