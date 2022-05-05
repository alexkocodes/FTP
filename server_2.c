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
#define MAX_USER 2

//a function to handle client connections that connect to server -> parameter: client descriptor and user array
void handle_connection(int client_sd, struct user *user_array){
    return;
}

//a function to close client connections that connect to server -> parameter: client descriptor, user array and current set of file descriptors
void close_client_connection(int client_sd, struct user *user_array, fd_set *currentfd){
    return;
}


typedef struct {
	char *username;
	char *password;
	int sock_num;
    int login_status;
}Users;

int main()
{

    // creating user profiles
	Users users[MAX_USER];


    //working with this now; waiting for file reading system to be made
    users[0].username = "bob";
    users[0].password = "donuts";
    users[0].sock_num = 0;
    users[0].login_status = 0;

    users[1].username = "armaan";
    users[1].password = "pizza";
    users[1].sock_num = 0;
    users[1].login_status = 0;


    // -----------------------FILE READING FROM users.txt------------------------

    // FILE *user_file = fopen("users.txt","r");
    // if (user_file == NULL) {
    //   perror("User File does not exist.");
    //   return 0;
    // }

    // while(fread(&users, sizeof(Users), 1, user_file))
    // {    printf ("Username = %s Password = %s\n", users.username,users.password);
    // }
    
    
    // int i = 0;
    // while (fscanf(user_file, "%s %s", users[i].username, users[i].password) == 2)
    // ++i;

    // for(int i =0; i<2; i++){
    //     Users *u = &users[i];
    //     printf("User: %s, Pass: %s, Socket: %d, Login: %d\n", u->username, u->password, u->sock_num, u->login_status);
    // }
    // char user_buffer[256];
    // int i = 0;
    // while (fgets(user_buffer, 256, user_file))
    // {
    //     // Remove trailing newline
    //     user_buffer[strcspn(user_buffer, "\n")] = 0;

    //     char username_string[256];
    //     char delim[] = " ";
    //     char password_string[256];
    //     char *ptr = strtok(user_buffer,delim);
    //     strcpy(username_string,ptr);
    //     ptr = strtok(NULL,delim);
    //     strcpy(password_string,ptr);
        
        
        
    //     char delim_2[] = ":";
        
    //     char *name_ptr = strtok(username_string,delim_2);
    //     name_ptr = strtok(NULL,delim_2);
    //     char username[256];
    //     strcpy(username,name_ptr);
        

    //     char *pass_ptr = strtok(password_string,delim_2);
    //     pass_ptr = strtok(NULL,delim_2);
    //     char password[256];
    //     strcpy(password,pass_ptr);

        
        
    //     memmove(users[i].username, username, strlen(username));
    //     users[i].username[strlen(username)] = '\0';

    //     users[i].password = password;
    //     users[i].sock_num = 0;
    //     users[i].login_status = 0;

    //     i++;

        

    //     // bzero(&user_buffer,sizeof(user_buffer));
    // }

    // for (int i =0; i<2; i++){
    //     printf("User: %s, Pass: %s, Socket: %d, Login: %d\n", users[i].username, users[i].password, users[i].sock_num, users[i].login_status);
    // }

    // fclose(user_file);

    //---------------------------------------------------------------------


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
	server_address.sin_port = htons(21);	//port
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
					handle_connection(i,users);
				}
			}
		}
	}	

	close(server_fd);	

    return 0;
}







//tasks to do:



//make a user data structure and read from users.txt

//make the main server socket 

//make a system to accepts concurrent clients using select

//implement the method that handles client connections
    //a.should respond appropriately to all commands
    //b.should open data connection with port 20 in case of RETR, STOR, LIST 
    //c.closes client connection