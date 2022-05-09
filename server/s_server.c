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


struct Users {
	char *username;
	char *password;
	int sock_num; //to ensure each authentication is independent
    int login_status; //0 -> not logged in ; 1 -> username entered; 2 -> logged in
};


struct arg_struct {
    int arg1;
    struct Users *arg2;
};

int client_data_port = 0;
char client_data_ip[256];
//a function to close client connections that connect to server -> parameter: client descriptor, user array and current set of file descriptors
void close_client_connection(int client_sd, struct Users user_array[], int *socket_list){

    
	for (int i = 0; i < MAX_SOCKET_NUM; i++)
	{
		if (socket_list[i] == client_sd)
		{
			socket_list[i] = 0;
		}
	}
    close(client_sd);
	for (int i = 0; i < MAX_USER; i++)
	{
		if (user_array[i].sock_num == client_sd)
		{
			user_array[i].login_status = 0;
			user_array[i].sock_num = 0;
			printf("User %s disconnected.\n", user_array[i].username);
		}
	}
    return;
}
 

//a function to handle client connections that connect to server -> parameter: client descriptor and user array
void handle_connection(int client_sd, struct Users user_array[MAX_USER], int *socket_list){

    // struct arg_struct *args = arguments;
    // // int client_sd = *(int*)client_sd_pointer;
	// free(client_sd_pointer);

    // printf("in handle conncetions\n");
    int authentication_status = 0; //0-> Not logged in; 1 -> Logged In.
    char client_request[256];
    bzero(&client_request, sizeof(client_request));

    char server_response[256];
    bzero(&server_response, sizeof(server_response));

    for (int i = 0; i < MAX_USER; i++)
	{	
		if (user_array[i].sock_num == client_sd && user_array[i].login_status == 2)
		{
			authentication_status = 1;
		}
	}
    

    if (recv(client_sd, client_request, sizeof(client_request) - 1, 0) > 0)
	{
        printf("Client req is %s: \n",client_request);
        char delim[] = " ";
        char *cptr = strtok(client_request,delim);
        char client_command[256];
        strcpy(client_command,cptr);


        printf("Client command is %s: \n",client_command);
        

        if(authentication_status == 0) //not logged in 
        {
                    if(strncmp(client_command, "USER",4) == 0 || strncmp(client_command, "user",4) == 0)
                    {
                        cptr = strtok(NULL,delim);
                        char username[256];
                        strcpy(username,cptr);
                        int user_existence = 0; // 0 -> Non-existent ; 1 -> Exists
                        for (int i = 0; i < MAX_USER; i++)
                        {	
                            if (strcmp(user_array[i].username, username) == 0)
                            {
                                if(user_array[i].login_status == 0 || user_array[i].login_status == 1)
                                {
                                    user_array[i].sock_num = client_sd;
                                    user_array[i].login_status = 1;
                                    user_existence = 1;
                                    strcpy(server_response,"331 Username OK, need password.");
                                    send(client_sd,server_response,strlen(server_response),0);
                                    bzero(&server_response, sizeof(server_response));
                                }
                                else
                                {
                                    bzero(&server_response, sizeof(server_response));
                                    strcpy(server_response,"230 User logged in, proceed.");
                                    send(client_sd,server_response,strlen(server_response),0);
                                }
                               
                            }
                        }
                        if(user_existence == 0){
                            strcpy(server_response,"530 Not logged in.");
                            printf("Not logged in: %s & %lu",server_response,sizeof(server_response));
                            send(client_sd,server_response,strlen(server_response),0);
                        }
                    }
                    else if(strncmp(client_command, "PASS",4) == 0 ||  strncmp(client_command, "pass",4) == 0)
                    {
                        cptr = strtok(NULL,delim);
                        char password[256];
                        strcpy(password,cptr);
                        for (int i = 0; i < MAX_USER; i++)
                        {	
                            if ((user_array[i].login_status == 1 || user_array[i].login_status == 2) && user_array[i].sock_num == client_sd)
                            {
                                if(strcmp(user_array[i].password,password) == 0)
                                {
                                    user_array[i].login_status = 2;
                                    strcpy(server_response,"230 User logged in, proceed.");
                                    send(client_sd,server_response,strlen(server_response),0);
                                    bzero(&server_response, sizeof(server_response));
                                }
                                else
                                {
                                    bzero(&server_response, sizeof(server_response));
                                    strcpy(server_response,"530 Not logged in.");
                                    send(client_sd,server_response,strlen(server_response),0);
                                }
                                return;
                            }
                        }
                        bzero(&server_response, sizeof(server_response));
                        strcpy(server_response,"530 Not logged in.");
                        send(client_sd,server_response,strlen(server_response),0);
                    }
                    else if((strcmp(client_command, "QUIT") == 0 ||  strcmp(client_command, "quit") == 0))
                    {
                        strcpy(server_response,"221 Service closing control connection.");
                        send(client_sd,server_response,strlen(server_response),0);
                        close_client_connection(client_sd,user_array, socket_list);
                    }
                    else{
                        strcpy(server_response,"530 Not logged in.");
                        send(client_sd,server_response,strlen(server_response),0);
                    }
        }
        else    //logged in
        {
            if(strncmp(client_command, "CWD",3) == 0 ||  strncmp(client_command, "cwd",3) == 0)
            {
                cptr = strtok(NULL,delim);
                char foldername[256];
                strcpy(foldername,cptr);
                foldername[strcspn(foldername, "\r\n")] = 0; //removing carriage return
                bzero(&server_response, sizeof(server_response));
                if (chdir(foldername) == 0){
                    strcpy(server_response,"200 directory changed to pathname/foldername.\n");
                    strcat(server_response,foldername);
                    // send(client_sd,foldername,strlen(foldername),0);
                    // system("pwd");
                }
                else{
                    strcpy(server_response,"550 No such file or directory.");
                    // send(client_sd,server_response,strlen(server_response),0);
                    // strcat(server_response,foldername);
                }
            
                send(client_sd,server_response,strlen(server_response),0);
            }
            else if(strncmp(client_command, "PWD",3) == 0 ||  strncmp(client_command, "pwd",3) == 0)
            {
                char wd[256];
                getcwd(wd, sizeof(wd));
                bzero(&server_response, sizeof(server_response));
                strcpy(server_response,"257 pathname.\n");
                strcat(server_response,wd);
                send(client_sd,server_response,strlen(server_response),0);
            }
            else if(strncmp(client_command, "PORT",3) == 0 ||  strncmp(client_command, "port",3) == 0)
            {
                cptr = strtok(NULL,delim);
                char data_socket[256];
                strcpy(data_socket,cptr);
                int p1;
                int p2;
                char delim_2[] = ",";
                char dot = '.';
                
                char *dptr = strtok(data_socket,delim_2);
                strcpy(client_data_ip,dptr);
                strncat(client_data_ip,&dot,1);
                dptr = strtok(NULL,delim_2);
                strcat(client_data_ip,dptr);
                strncat(client_data_ip,&dot,1);
                dptr = strtok(NULL,delim_2);
                strcat(client_data_ip,dptr);
                strncat(client_data_ip,&dot,1);
                dptr = strtok(NULL,delim_2);
                strcat(client_data_ip,dptr);

                dptr = strtok(NULL,delim_2);
                p1 = atoi(dptr);
                dptr = strtok(NULL,delim_2);
                p2 = atoi(dptr);

                client_data_port = (p1*256)+p2;
                printf("Client IP: %s and Port: %d\n",client_data_ip,client_data_port);

                strcpy(server_response,"200 PORT command successful.");
                send(client_sd,server_response,strlen(server_response),0);
                bzero(&server_response,sizeof(server_response));
            }
            else if(strncmp(client_command, "RETR",4) == 0 ||  strncmp(client_command, "retr",4) == 0) //file sent from server to client
            {
                int pid = fork();
                if (pid == 0)
                {
                    cptr = strtok(NULL,delim);
                    char filename[256];
                    strcpy(filename,cptr);
                    FILE *fptr = fopen(filename,"r");		//open requested file
                    if(fptr == NULL) //serving error message in case file not present in disk
                    {	
                        strcpy(server_response,"550 No such file or directory.");  
                        return;
                    }
                    else
                    {
                        strcpy(server_response,"150 File status okay; about to open data connection.");   
                        send(client_sd,server_response,strlen(server_response),0);


                        int data_fd = socket(AF_INET, SOCK_STREAM, 0);
                        if (data_fd < 0)
                        {
                            perror("Error opening Socket: Server Data.");
                            return;
                        }

                        //building socket's Internet Address
                        struct sockaddr_in client_data_addr,server_data_addr;

                        bzero(&client_data_addr,sizeof(client_data_addr));
                        client_data_addr.sin_family = AF_INET;
                        client_data_addr.sin_port = htons(client_data_port);
                        inet_pton(AF_INET, client_data_ip, &(client_data_addr.sin_addr));

                        bzero(&server_data_addr,sizeof(server_data_addr));
                        server_data_addr.sin_family = AF_INET;	//address family
                        server_data_addr.sin_port = htons(9001);	//using port 9000 because port 20 is priveledged
                        server_data_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

                        char server_data_ip[INET_ADDRSTRLEN]; 
                        inet_ntop(AF_INET, &(client_data_addr.sin_addr), server_data_ip, INET_ADDRSTRLEN);
                        printf("Server is connecting to IP: %s and Port: %d\n",client_data_ip,ntohs(client_data_addr.sin_port));
                        //connecting to server socket

                        if (bind(data_fd, (struct sockaddr*) &server_data_addr, sizeof(struct sockaddr_in)) == 0)
                            printf("Server Data binded Correctly\n");
                        else
                            printf("Server Data unable to bind\n");

                        if(connect(data_fd,(struct sockaddr*)&client_data_addr,sizeof(client_data_addr))<0)
                        {
                            perror("Connection Failed: Server Data.");
                            exit(-1);
                        }

                        bzero(&server_response,sizeof(server_response));
                        char line[256];
                        while (fgets(line, sizeof(line), fptr) != NULL) 
                        {
                            
                            if (send(data_fd, line, sizeof(line), 0) == -1) 
                            {
                                perror("Error Sending file..\n");
                                return;
                            }
                            memset(line, 0, sizeof(line));
                        }
                        fclose(fptr);

                        strcat(server_response,"226 Transfer completed.");
                        send(data_fd,server_response,strlen(server_response),0);
                        close(data_fd);
                    }
                }
            }
            else if(strncmp(client_command, "STOR",4) == 0 ||  strncmp(client_command, "stor",4) == 0) //file sent from client to server
            {
                int pid = fork();
                if (pid == 0)
                {
                    cptr = strtok(NULL,delim);
                    char filename[256];
                    strcpy(filename,cptr);
                    FILE *fptr = fopen(filename,"w");		//open requested file
                    if(fptr == NULL) //serving error message in case file not present in disk
                    {	
                        strcpy(server_response,"550 No such file or directory.");  
                        return;
                    }
                    else
                    {
                        strcpy(server_response,"150 File status okay; about to open data connection.");   
                        send(client_sd,server_response,strlen(server_response),0);


                        int data_fd = socket(AF_INET, SOCK_STREAM, 0);
                        if (data_fd < 0)
                        {
                            perror("Error opening Socket: Server Data.");
                            return;
                        }

                        //building socket's Internet Address
                        struct sockaddr_in client_data_addr,server_data_addr;

                        bzero(&client_data_addr,sizeof(client_data_addr));
                        client_data_addr.sin_family = AF_INET;
                        client_data_addr.sin_port = htons(client_data_port);
                        inet_pton(AF_INET, client_data_ip, &(client_data_addr.sin_addr));

                        bzero(&server_data_addr,sizeof(server_data_addr));
                        server_data_addr.sin_family = AF_INET;	//address family
                        server_data_addr.sin_port = htons(9001);	//using port 9000 because port 20 is priveledged
                        server_data_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

                        char server_data_ip[INET_ADDRSTRLEN]; 
                        inet_ntop(AF_INET, &(client_data_addr.sin_addr), server_data_ip, INET_ADDRSTRLEN);
                        printf("Server is connecting to IP: %s and Port: %d\n",client_data_ip,ntohs(client_data_addr.sin_port));
                        //connecting to server socket

                        if (bind(data_fd, (struct sockaddr*) &server_data_addr, sizeof(struct sockaddr_in)) == 0)
                            printf("Server Data binded Correctly\n");
                        else
                            printf("Server Data unable to bind\n");

                        if(connect(data_fd,(struct sockaddr*)&client_data_addr,sizeof(client_data_addr))<0)
                        {
                            perror("Connection Failed: Server Data.");
                            exit(-1);
                        }

                        bzero(&server_response,sizeof(server_response));
                        
                        
                        char server_data[256];

                        int server_return_value = 0;
                        memset(server_data, 0, sizeof(server_data));
                        while ((server_return_value = recv(data_fd, server_data, sizeof(server_data), 0)) > 0)
                        {
                            fputs(server_data, fptr);
                            memset(server_data, 0, sizeof(server_data));
                        }

                        fclose(fptr);
                        

                        fflush(stdout);
                        strcat(server_response,"226 Transfer completed.");
                        send(data_fd,server_response,strlen(server_response),0);
                        close(data_fd);
                    }
                }
            }
            else if(strncmp(client_command, "LIST",4) == 0 ||  strncmp(client_command, "list",4) == 0)
            {
                int pid = fork();
                if (pid == 0)
                {
                    strcpy(server_response,"150 File status okay; about to open data connection.");   
                    send(client_sd,server_response,strlen(server_response),0);


                    int data_fd = socket(AF_INET, SOCK_STREAM, 0);
                    if (data_fd < 0)
                    {
                        perror("Error opening Socket: Server Data.");
                        return;
                    }

                    //building socket's Internet Address
                    struct sockaddr_in client_data_addr,server_data_addr;

                    bzero(&client_data_addr,sizeof(client_data_addr));
                    client_data_addr.sin_family = AF_INET;
                    client_data_addr.sin_port = htons(client_data_port);
                    inet_pton(AF_INET, client_data_ip, &(client_data_addr.sin_addr));

                    bzero(&server_data_addr,sizeof(server_data_addr));
                    server_data_addr.sin_family = AF_INET;	//address family
                    server_data_addr.sin_port = htons(9001);	//using port 9000 because port 20 is priveledged
                    server_data_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

                    char server_data_ip[INET_ADDRSTRLEN]; 
                    inet_ntop(AF_INET, &(client_data_addr.sin_addr), server_data_ip, INET_ADDRSTRLEN);
                    printf("Server is connecting to IP: %s and Port: %d\n",client_data_ip,ntohs(client_data_addr.sin_port));
                    //connecting to server socket

                    if (bind(data_fd, (struct sockaddr*) &server_data_addr, sizeof(struct sockaddr_in)) == 0)
                        printf("Server Data binded Correctly\n");
                    else
                        printf("Server Data unable to bind\n");

                    if(connect(data_fd,(struct sockaddr*)&client_data_addr,sizeof(client_data_addr))<0)
                    {
                        perror("Connection Failed: Server Data.");
                        exit(-1);
                    }

                    bzero(&server_response,sizeof(server_response));
                    char file_line[128];
                    FILE *list_file = popen("ls", "r");
                    if (list_file)
                    {
                        while (fgets(file_line, sizeof(file_line), list_file))
                        {
                            strcat(server_response, file_line);
                        }
                        pclose(list_file);
                    }
                    strcat(server_response,"226 Transfer completed.");
                    send(data_fd,server_response,strlen(server_response),0);
                    close(data_fd);
                }
            }
            
            else if((strcmp(client_command, "QUIT") == 0 ||  strcmp(client_command, "quit") == 0))
            {
                strcpy(server_response,"221 Service closing control connection.");
                send(client_sd,server_response,strlen(server_response),0);
                close_client_connection(client_sd,user_array, socket_list);
            }
            else{
                strcpy(server_response,"202 Command not implemented.");
                send(client_sd,server_response,strlen(server_response),0);
            }
        }

        
        
        
    }  
    bzero(&client_request,sizeof(client_request));
    bzero(&server_response, sizeof(server_response));
    return;
}
// void handle_connection(void *arguments);







int main()
{

    // creating user profiles
	// struct Users users[MAX_USER];


    // //working with this now; waiting for file reading system to be made
    // users[0].username = "bob";
    // users[0].password = "donuts";
    // users[0].sock_num = 0;
    // users[0].login_status = 0;

    // users[1].username = "armaan";
    // users[1].password = "pizza";
    // users[1].sock_num = 0;
    // users[1].login_status = 0;

    char *filename = "users.txt";
    FILE *fp = fopen(filename, "r");

    if (fp == NULL)
    {
        printf("Error: could not open file %s", filename);
        return 1;
    }

    // reading line by line, max 256 bytes
    char buffer[256];
	
	struct Users* users = malloc(MAX_USER * sizeof(*users));
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
				users[counter].username = malloc(128); // allocate memory for username
				strcpy(users[counter].username, token);
			}
			else if(i == 1){
				users[counter].password = malloc(128); // allocate memory for password
				strcpy(users[counter].password, token);	
			}
			token = strtok(NULL, " ");
			i += 1;
			
		}
		users[counter].sock_num = 0;
		users[counter].login_status = 0;
		counter += 1;
   	}

	for(int i=0; i<counter; i++){
		printf( "Username: %s\n", users[i].username ); //printing each user's username
		printf( "Password: %s\n", users[i].password ); //printing each user's password
	}
	// when to free them?



    // close the file
    fclose(fp);

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
	server_address.sin_port = htons(9001);	//using 9001 port because 21 is priviledged
	server_address.sin_addr.s_addr = inet_addr("127.0.0.1"); //htonl(INADDR_LOOPBACK); //inet_addr("127.0.0.1");

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

	char server_ip[INET_ADDRSTRLEN]; 
    inet_ntop(AF_INET, &(server_address.sin_addr), server_ip, INET_ADDRSTRLEN);
    printf("Server accepts to IP: %s and Port: %d\n",server_ip,ntohs(server_address.sin_port));

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
		for (int i=0; i<=max_fd; i++){
			if(FD_ISSET(i, &socks))
            {
				if(i == server_fd)
                { // if it's the server socket chosen, it means that there's a new connection coming
					int client_sd = accept(server_fd,NULL,NULL);
					if(client_sd<1)
					{
						perror("Accept Error:");
						return -1;
					}
                    // printf("Client Entered: %d\n",client_sd);
					FD_SET(client_sd, &socks);
					if(client_sd> max_fd){ // update max socket bit in the set
						// max_fd = socket_list[i];
                        max_fd = client_sd;
                        // printf("Hello\n");
					}

                    // for(int i = 0; i<MAX_SOCKET_NUM; i++){
                    //     printf("Socket Num: %d is %d\n",i,socket_list[i]);
                    // }
					// find the first 0 and set it to client_sd
					for(int i=0; i<MAX_SOCKET_NUM; i++)
                    {
						if(socket_list[i]==0)
                        {
                            
							socket_list[i] = client_sd;
                            printf("\nSocket Num: %d is %d\n",i,socket_list[i]);
						    printf("New Connection established!\n");
						    break;
                        }
                        else{
                            printf("noope.\n");
                        }
					}
					break;
				}
				else
                { // if it's not server, start serving files
					handle_connection(i,users,socket_list);
                    FD_CLR(i, &socks);
				}
			}
		}
	}	

        // int client_sd = accept(server_fd,NULL,NULL);
            
        // if(client_sd<1)
        // {
        //     perror("Accept Error:");
        //     return -1;
        // }
        // else
        // {
        //     // implementing threading to handle connection, serve files and eventually close connection
        //     int* client_pointer = malloc(sizeof(int));
        //     *client_pointer = client_sd;

        //     struct arg_struct args;
        //     args.arg1 = client_sd;
        //     args.arg2 = users;
        //     pthread_t thread;
        //     if(pthread_create(&thread,NULL,handle_connection,(void *)&args) != 0){
        //         printf("Uh-oh!\n");
        //         return -1;
        //     };
            
        // }

        // int client_sd = accept(server_fd,NULL,NULL);
		
		// int pid = fork(); //fork a child process
        // if (pid == 0){
        //     // struct arg_struct args;
        //     // args.arg1 = client_sd;
        //     // args.arg2 = users;
        //     handle_connection(client_sd, users);
        // }
    

	close(server_fd);	

    return 0;
}





// void handle_connection(void *arguments){




//tasks to do:



//make a user data structure and read from users.txt

//make the main server socket 

//make a system to accepts concurrent clients using select

//implement the method that handles client connections
    //a.should respond appropriately to all commands
    //b.should open data connection with port 20 in case of RETR, STOR, LIST 
    //c.closes client connection