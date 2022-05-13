//FTP Server
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
#include <sys/mman.h>

#define MAX_SOCKET_NUM 10
#define MAX_USER 2

//the user structure to store username, password, client socket descriptor associated with the user and its login status
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

//variable to store client's data port and IP
int client_data_port = 0;

char client_data_ip[256];

// a function to handle list command in thread
void *handle_list(void *arg){

    char server_response[256];

    int client_sd = *(int*)(arg);
    strcpy(server_response,"150 File status okay; about to open data connection.");   
    send(client_sd,server_response,strlen(server_response),0);

    int data_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (data_fd < 0)
    {
        perror("Error opening Socket: Server Data.");
        return NULL;
    }

    
    //building data socket's Internet Address using received client port and IP
    struct sockaddr_in client_data_addr,server_data_addr;

    bzero(&client_data_addr,sizeof(client_data_addr));
    client_data_addr.sin_family = AF_INET;
    client_data_addr.sin_port = htons(client_data_port);
    client_data_addr.sin_addr.s_addr = inet_addr(client_data_ip);
   

    bzero(&server_data_addr,sizeof(server_data_addr));
    server_data_addr.sin_family = AF_INET;	//address family
    server_data_addr.sin_port = htons(9000);	//using port 9000 because port 20 is priveledged
    server_data_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    char server_data_ip[INET_ADDRSTRLEN]; 
    inet_ntop(AF_INET, &(client_data_addr.sin_addr), server_data_ip, INET_ADDRSTRLEN);
    printf("Server is connecting to IP: %s and Port: %d\n",client_data_ip,htons(client_data_port));
    printf("Server time %lu \n",time(NULL));
    //connecting to server socket
    int value =1;
    setsockopt(data_fd,SOL_SOCKET,SO_LINGER,&value,sizeof(value));
    if (bind(data_fd, (struct sockaddr*) &server_data_addr, sizeof(struct sockaddr_in)) == 0)
        printf("Server Data binded Correctly\n");
    else
    {
        
        printf("Server Data unable to bind\n");
    }


    if(connect(data_fd,(struct sockaddr*)&client_data_addr,sizeof(client_data_addr))<0)
    {
        perror("Connection Failed: Server Data.");
        
        exit(-1);
    }

    bzero(&server_response,sizeof(server_response));
    char file_line[256];
    //creating a pipe to send back the list of files in the server directory to the client
    FILE *list_file = popen("ls", "r");
    if (list_file)
    {
        while (fgets(file_line, sizeof(file_line), list_file))
        {
            if (send(data_fd,file_line,strlen(file_line),0) < 0)
            {
                perror("Error Sending file..\n");
                return 0;
            }
            bzero(&file_line,sizeof(file_line));
            fflush(stdout);
            
        }
        pclose(list_file);
    }
    strcat(server_response,"226 Transfer completed.");
    send(client_sd,server_response,strlen(server_response),0);

    
    if(close(data_fd)<0){
        perror("close error\n");
    };

    return NULL;
}
//a function to close client connections that connect to server -> parameter: client descriptor, user array
void close_client_connection(int client_sd, struct Users user_array[]){

    
	
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
void handle_connection(int client_sd, struct Users user_array[MAX_USER]){

    
    int authentication_status = 0; //0-> Not logged in; 1 -> Logged In.
    char client_request[256];
    bzero(&client_request, sizeof(client_request));

    char server_response[256];
    bzero(&server_response, sizeof(server_response));

    
    
    while(1)
    {
        //see if there is an authenticated user to change authentication status for the session
        for (int i = 0; i < MAX_USER; i++)
        {	
            if (user_array[i].sock_num == client_sd && user_array[i].login_status == 2)
            {
                authentication_status = 1;
            }
        }
        bzero(&client_request, sizeof(client_request));
    if (recv(client_sd, client_request, sizeof(client_request) - 1, 0) > 0) //recieve input from client
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
                        if(cptr == NULL){
                             // in case no username entered along with user command
                            bzero(&server_response, sizeof(server_response));
                            strcpy(server_response,"530 Not logged in.");
                            send(client_sd,server_response,strlen(server_response),0);
                        }
                        else
                        {
                            char username[256];
                            strcpy(username,cptr);
                            int user_existence = 0; // 0 -> Non-existent ; 1 -> Exists
                            for (int i = 0; i < MAX_USER; i++)
                            {	
                                //if username matches and user is not logged in, change login status to 1 and update sock num with client_sd 
                                if (strcmp(user_array[i].username, username) == 0)
                                {
                                    if(user_array[i].login_status == 0 || user_array[i].login_status == 1)
                                    {
                                        user_array[i].sock_num = client_sd;
                                        user_array[i].login_status = 1;
                                        for (int i = 0; i < MAX_USER; i++)
                                        {	
                                            if (strcmp(user_array[i].username, username) != 0)
                                            {
                                                user_array[i].sock_num = 0;
                                                user_array[i].login_status = 0;
                                            }
                                        }
                                        user_existence = 1;
                                        strcpy(server_response,"331 Username OK, need password.");
                                        send(client_sd,server_response,strlen(server_response),0);
                                        bzero(&server_response, sizeof(server_response));
                                    }
                                    else
                                    {
                                        //user already logged in
                                        bzero(&server_response, sizeof(server_response));
                                        strcpy(server_response,"230 User logged in, proceed.");
                                        send(client_sd,server_response,strlen(server_response),0);
                                    }
                                
                                }
                            }
                            //in case it is a wrong username 
                            if(user_existence == 0){
                                strcpy(server_response,"530 Not logged in.");
                                printf("Not logged in: %s & %lu",server_response,sizeof(server_response));
                                send(client_sd,server_response,strlen(server_response),0);
                            }
                        }
                    }
                    else if(strncmp(client_command, "PASS",4) == 0 ||  strncmp(client_command, "pass",4) == 0)
                    {
                        cptr = strtok(NULL,delim);
                        if(cptr != NULL)
                        {
                        
                            char password[256];
                            strcpy(password,cptr);
                            int valid_user = 0; // 0 -> Invalid User 1 -> Valid User
                            for (int i = 0; i < MAX_USER; i++)
                            {	
                                if ((user_array[i].login_status == 1 || user_array[i].login_status == 2) && user_array[i].sock_num == client_sd)
                                {
                                    //if right password
                                    if(strcmp(user_array[i].password,password) == 0)
                                    {
                                        user_array[i].login_status = 2;
                                        strcpy(server_response,"230 User logged in, proceed.");
                                        send(client_sd,server_response,strlen(server_response),0);
                                        bzero(&server_response, sizeof(server_response));
                                        valid_user = 1;
                                    }
                                   
                                }
                            }
                            //if password is wrong
                            if(valid_user == 0)
                            {
                                bzero(&server_response, sizeof(server_response));
                                strcpy(server_response,"530 Not logged in.");
                                send(client_sd,server_response,strlen(server_response),0);
                            }
                                
                        }
                        else{
                            // in case no password entered along with pass command
                            bzero(&server_response, sizeof(server_response));
                            strcpy(server_response,"530 Not logged in.");
                            send(client_sd,server_response,strlen(server_response),0);
                        }
                    }
                    else if((strcmp(client_command, "QUIT") == 0 ||  strcmp(client_command, "quit") == 0))
                    {
                        //close client connections upon quit
                        strcpy(server_response,"221 Service closing control connection.");
                        send(client_sd,server_response,strlen(server_response),0);
                        close_client_connection(client_sd,user_array); //, socket_list
                        return;
                    }
                    else{
                        //if any other command entered and the user has not logged in
                        strcpy(server_response,"530 Not logged in.");
                        send(client_sd,server_response,strlen(server_response),0);
                    }
        }
        else    //logged in
        {
            if(strncmp(client_command, "CWD",3) == 0 ||  strncmp(client_command, "cwd",3) == 0)
            {
                cptr = strtok(NULL,delim);
                if(cptr == NULL){
                    // in case no folder name is entered
                    bzero(&server_response, sizeof(server_response));
                    strcpy(server_response,"550 No such file or directory.");
                }
                else
                {
                    char foldername[256];
                    strcpy(foldername,cptr);
                    foldername[strcspn(foldername, "\r\n")] = 0; //removing carriage return
                    bzero(&server_response, sizeof(server_response));
                    //change the directory to the foldername specified
                    if (chdir(foldername) == 0){
                    
                        strcpy(server_response,"200 directory changed to pathname/foldername.\n");
                        strcat(server_response,foldername);
                        
                    }
                    else{
                        // in case of non existing folder
                        strcpy(server_response,"550 No such file or directory.");
                        
                    }
                }
            
                send(client_sd,server_response,strlen(server_response),0);
            }
            else if(strncmp(client_command, "PWD",3) == 0 ||  strncmp(client_command, "pwd",3) == 0)
            {
                //get the current working directory and print it out
                char wd[256];
                getcwd(wd, sizeof(wd));
                bzero(&server_response, sizeof(server_response));
                strcpy(server_response,"257 pathname.\n");
                strcat(server_response,wd);
                send(client_sd,server_response,strlen(server_response),0);
                
            }
            else if(strncmp(client_command, "USER",3) == 0 ||  strncmp(client_command, "user",3) == 0)
            {
                //if user command entered after loggin in, just say User is alredy logged in
                bzero(&server_response, sizeof(server_response));
                strcpy(server_response,"230 User logged in, proceed.");
                send(client_sd,server_response,strlen(server_response),0);
                
            }
            else if(strncmp(client_command, "PASS",3) == 0 ||  strncmp(client_command, "pass",3) == 0)
            {
                //if pass command entered after loggin in, just say User is alredy logged in
                bzero(&server_response, sizeof(server_response));
                strcpy(server_response,"230 User logged in, proceed.");
                send(client_sd,server_response,strlen(server_response),0);
            }
            else if(strncmp(client_command, "PORT",3) == 0 ||  strncmp(client_command, "port",3) == 0)
            {
                //build the client IP and port from the port command and store it in the respective variables
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

                printf("Client IP: %s and Port: %d\n",client_data_ip,htons(client_data_port));

                strcpy(server_response,"200 PORT command successful.");
                send(client_sd,server_response,strlen(server_response),0);
                bzero(&server_response,sizeof(server_response));
            }
            else if(strncmp(client_command, "RETR",4) == 0 ||  strncmp(client_command, "retr",4) == 0) //file sent from server to client
            {
                //fork a process in order to concurrently in order to not let file transfers block the system
                int pid = fork();
                if (pid == 0)
                {
                    cptr = strtok(NULL,delim);
                    if(cptr != NULL)
                    {
                        char filename[256];
                        strcpy(filename,cptr);
                        FILE *fptr = fopen(filename,"r");		//open requested file
                        if(fptr == NULL) //serving error message in case file not present in disk
                        {	
                            bzero(&server_response,sizeof(server_response));
                            strcpy(server_response,"550 No such file or directory.");  
                            send(client_sd,server_response,strlen(server_response),0);
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

                            //building data socket's Internet Address using the client port and IP received 
                            struct sockaddr_in client_data_addr,server_data_addr;

                            bzero(&client_data_addr,sizeof(client_data_addr));
                            client_data_addr.sin_family = AF_INET;
                            client_data_addr.sin_port = htons(client_data_port);
                            inet_pton(AF_INET, client_data_ip, &(client_data_addr.sin_addr));

                            bzero(&server_data_addr,sizeof(server_data_addr));
                            server_data_addr.sin_family = AF_INET;	//address family
                            server_data_addr.sin_port = htons(9000);	//using port 9000 because port 20 is priveledged
                            server_data_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

                            char server_data_ip[INET_ADDRSTRLEN]; 
                            inet_ntop(AF_INET, &(client_data_addr.sin_addr), server_data_ip, INET_ADDRSTRLEN);
                            printf("Server is connecting to IP: %s and Port: %d\n",client_data_ip,htons(client_data_port));
                            //connecting to server socket
                            int value = 1;
                            setsockopt(data_fd,SOL_SOCKET,SO_LINGER,&value,sizeof(value));
                            if (bind(data_fd, (struct sockaddr*) &server_data_addr, sizeof(struct sockaddr_in)) == 0)
                                printf("Server Data binded Correctly\n");
                            else
                                perror("Server Data unable to bind\n");

                            if(connect(data_fd,(struct sockaddr*)&client_data_addr,sizeof(client_data_addr))<0)
                            {
                                perror("Connection Failed: Server Data.");
                                exit(-1);
                            }
                            printf("Server Filename: %s\n", filename);
                            bzero(&server_response,sizeof(server_response));
                           
                           //send data through the data channel
                            while(1)
                            {
                                /* First read file in chunks of 256 bytes */
                                unsigned char buff[1024]={0};
                                int nread = fread(buff,1,1024,fptr);
                                   

                                /* If read was success, send data. */
                                if(nread > 0)
                                {
                                    
                                    write(data_fd, buff, nread);
                                }
                                if (nread < 1024)
                                {
                                    if (feof(fptr))
                                    {
                                        printf("End of file\n");
                                        printf("File transfer completed for id: %d\n",data_fd);
                                    }
                                    if (ferror(fptr))
                                        printf("Error reading\n");
                                    break;
                                }
                            }
                            fclose(fptr);

                            strcat(server_response,"226 Transfer completed.");
                            send(client_sd,server_response,strlen(server_response),0);
                            close(data_fd);
                            return;
                            // close(data_fd);
                        }
                    }
                    else{
                        //if invalid file
                        bzero(&server_response,sizeof(server_response));
                        strcpy(server_response,"550 No such file or directory.");  
                        send(client_sd,server_response,strlen(server_response),0);
                        exit(1);
                    }
                }
                else{
                    continue;
                }
            }
            else if(strncmp(client_command, "STOR",4) == 0 ||  strncmp(client_command, "stor",4) == 0) //file sent from client to server
            {
                //fork a process in order to concurrently in order to not let file transfers block the system
                int pid = fork();
                if (pid == 0)
                {
                    cptr = strtok(NULL,delim);
                    if(cptr != NULL)
                    {
                        strcpy(server_response,"150 File status okay; about to open data connection.");   
                        send(client_sd,server_response,strlen(server_response),0);
                        int data_fd = socket(AF_INET, SOCK_STREAM, 0);
                        if (data_fd < 0)
                        {
                            perror("Error opening Socket: Server Data.");
                            return;
                        }

                        //building data socket's Internet Address using the client port and IP received
                        struct sockaddr_in client_data_addr,server_data_addr;

                        bzero(&client_data_addr,sizeof(client_data_addr));
                        client_data_addr.sin_family = AF_INET;
                        client_data_addr.sin_port = htons(client_data_port);
                        inet_pton(AF_INET, client_data_ip, &(client_data_addr.sin_addr));

                        bzero(&server_data_addr,sizeof(server_data_addr));
                        server_data_addr.sin_family = AF_INET;	//address family
                        server_data_addr.sin_port = htons(9000);	//using port 9000 because port 20 is priveledged
                        server_data_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

                        char server_data_ip[INET_ADDRSTRLEN]; 
                        inet_ntop(AF_INET, &(client_data_addr.sin_addr), server_data_ip, INET_ADDRSTRLEN);
                        printf("Server is connecting to IP: %s and Port: %d\n",client_data_ip,htons(client_data_port));
                        //connecting to server socket
                        int value =1;
                        setsockopt(data_fd,SOL_SOCKET,SO_LINGER,&value,sizeof(value));
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
                            
                        
                        char filename[256];
                        strcpy(filename,cptr);

                        char server_file[256];
                        strcpy(server_file, "Server-"); //adding Server- in the front to mark it as the received file
                        strcat(server_file, filename);
                        FILE *fptr;		//create file to store data in
                        
                        int bytesReceived = 0;
                        char recvBuff[1024];
                        memset(recvBuff, '0', sizeof(recvBuff));
                        fptr = fopen(server_file, "w"); 
                        
                        if(NULL == fptr)
                        {
                            printf("Error opening file");
                            return;
                        }
                        else
                        {
                                
                        
                        
                            //reading and writing data
                            while((bytesReceived = read(data_fd, recvBuff, 1024)) > 0)
                            { 
                                
                                
                                fflush(stdout);
                                fwrite(recvBuff, 1,bytesReceived,fptr);
                                
                            }

                            if(bytesReceived < 0)
                            {
                                printf("\n Read Error \n");
                            }
                            printf("\nFile OK....Completed\n");
                            
                        }
                        fclose(fptr);
                        strcat(server_response,"226 Transfer completed.");
                        send(client_sd,server_response,strlen(server_response),0);
                        close(data_fd);
                        fflush(stdout);
                    }
                    else
                    {
                        //in case of invalid file
                        bzero(&server_response,sizeof(server_response));
                        strcpy(server_response,"550 No such file or directory.");  
                        send(client_sd,server_response,strlen(server_response),0);
                        exit(1);
                    }
                    
                }
                
            }
            else if(strncmp(client_command, "LIST",4) == 0 ||  strncmp(client_command, "list",4) == 0)
            {
                //create a thread to handle list command
                pthread_t t;
                pthread_create(&t, NULL, handle_list, &client_sd);
                
            }
            
            else if((strcmp(client_command, "QUIT") == 0 ||  strcmp(client_command, "quit") == 0))
            {
                //closing client connection when quit is specified
                strcpy(server_response,"221 Service closing control connection.");
                send(client_sd,server_response,strlen(server_response),0);
                close_client_connection(client_sd,user_array); //, socket_list
                return;
            }
            else{
                //in case of invalid command
                strcpy(server_response,"202 Command not implemented.");
                send(client_sd,server_response,strlen(server_response),0);
            }
        }
    }
        
        
        
    }  
    bzero(&client_request,sizeof(client_request));
    bzero(&server_response, sizeof(server_response));
    return;
}




int main()
{

    
    //server reads from users.txt and stores in struct user array
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

		

		// loop through this line to extract all other tokens
		while( token != NULL && strcmp(token, "\n") !=0 ) {
			
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

    // close the file
    fclose(fp);

    

    //Building the main server socket
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

        int client_sd = accept(server_fd,NULL,NULL);
		
		int pid = fork(); //fork a child process to handle multiple clients parallely
        if (pid == 0){
            handle_connection(client_sd, users); //function to handle communication with client
        }
    
    }
	close(server_fd);	
    //free the memory
	for(int i=0; i<counter; i++){
		free(users[i].username ); 
		free(users[i].password );
	}
    return 0;
}




