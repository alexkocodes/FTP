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




int main(int argc, char *argv[])
{   

    //Verifying that command line arguements include hostname and port number
    if (argc != 3){
        printf("Invalid number of arguements. Please only enter IP address and Port Number.");
        return 0;
    }
    else if (strlen(argv[1]) > 15){
        printf("Length of IP Address is invalid. Should be 15 characters or less.");
        return 0;
    }
    else if (strlen(argv[2]) > 5){
        printf("Length of Port Number is invalid. Should be 5 characters or less.");
        return 0;
    }


    //storing server hostname and port
    char server_ip[16];
    int server_port = atoi(argv[2]);

    strcpy(server_ip,argv[1]);

    printf("\nServer IP: %s and Port: %d\n",server_ip,server_port);



    //Socket Creation

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("Error opening Socket.");
        return (-1);
    }

    //building socket's Internet Address
    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &(server_addr.sin_addr));

    char client_ip[INET_ADDRSTRLEN]; 
    inet_ntop(AF_INET, &(server_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    printf("Client is connecting to IP: %s and Port: %d\n",client_ip,ntohs(server_addr.sin_port));
    //connecting to server socket
    if(connect(server_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        perror("Connection Failed.");
        exit(-1);
    }
    printf("Connected to server with IP: %s and Port: %d\n",server_ip,server_port);

    socklen_t len = sizeof(server_addr);
    if (getsockname(server_fd, (struct sockaddr *)&server_addr, &len) == -1)
    {
        perror("Error in getsockname");
        return 0;
    }

    //getting client port number
    
    int client_port_no = ntohs(server_addr.sin_port);

    
    printf("Client port number: %d\n",client_port_no);

    //Beginning of FTP User Interface

    //facilitating command response interaction between client and server
    char command[256];
    char response[512];

    // int print_it = 1;
    while(1){
        bzero(command,sizeof(command));
        bzero(&response,sizeof(response));
        
        // if(print_it)
        {
            printf("\nftp> ");
            fgets(command, sizeof(command), stdin); 
            command[strcspn(command, "\n")] = 0; //removing trailing \n in string
        }

        //QUIT command 
        if (strncmp(command, "QUIT",4) == 0 ||  strncmp(command, "quit",4) == 0){
            if(send(server_fd, command, strlen(command), 0) < 0){
                perror("Error in send.");
                exit(-1);
            };
            bzero(&response,sizeof(response));
            recv(server_fd, response, sizeof(response), 0);
            // printf("in the quit section\n");
            printf("%s\n", response);
            close(server_fd);
            return NULL;
        }


        //USER command
        else if(strncmp(command, "USER",4) == 0 ||  strncmp(command, "user",4) == 0){
            // printf("USER command used\n");
            if(send(server_fd, command, strlen(command), 0) < 0){
                perror("Error in send.");
                exit(-1);
            }
            bzero(&response,sizeof(response));
            recv(server_fd, response, sizeof(response), 0);
            printf("%s\n", response);
            fflush(stdout);
        }


        //PASS command
        else if(strncmp(command, "PASS",4) == 0 ||  strncmp(command, "pass",4) == 0){
            bzero(&response,sizeof(response));
            send(server_fd, command, strlen(command), 0);
            recv(server_fd, response, sizeof(response), 0);
            printf("%s\n", response);
        }


        //STOR command - upload
        else if(strncmp(command, "STOR",4) == 0 ||  strncmp(command, "stor",4) == 0){

            //need to upload file from current client directory to current server directory
            //things to do:
            
            //1.see if file exists
            char new_command[256];
            strcpy(new_command,command);
            char delim[] = " ";
            char *ptr = strtok(command,delim);
            ptr = strtok(NULL,delim);

            char filename[256];
            strcpy(filename,ptr);
            // printf('This is the filename: %s', filename);
            FILE *fptr = fopen(filename,"r");		//open requested file
            if(fptr == NULL) //serving error message in case file not present in disk
            {	
                perror("File does not exist.");
                continue;
            }
            
            else
            {

                

                //2.sending client port over to server using PORT command: Server responds with: 200 PORT command successful.
                // client_port_no++; //update client port no
                int p1,p2;
                p1 = client_port_no/256;
                p2 = client_port_no%256;
                char port_command[256] = "PORT 127,0,0,1,";
                char p_1[256];
                char p_2[256];
                sprintf(p_1, "%d", p1);
                sprintf(p_2, "%d", p2);
                strcat(port_command,p_1);
                strcat(port_command,",");
                strcat(port_command,p_2);
                bzero(&response,sizeof(response));
                send(server_fd, port_command, strlen(port_command), 0);
                recv(server_fd, response, sizeof(response), 0);
                if(strcmp(response,"200 PORT command successful.") == 0)
                {
                    printf("%s\n", response);
                }

                int pid = fork();
                if (pid == 0)
                {
                    int client_sender_sd = socket(AF_INET,SOCK_STREAM,0);
                    if(client_sender_sd<0)
                    {
                        perror("Client Receiver Socket creation:");
                        exit(-1);
                    }
                    //setsock
                    int value  = 1;
                    setsockopt(client_sender_sd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)); //&(int){1},sizeof(int)
                    struct sockaddr_in client_sender_addr, server_data_addr;

                    bzero(&client_sender_addr,sizeof(client_sender_addr));

                    client_sender_addr.sin_family = AF_INET;
                    client_sender_addr.sin_port = htons(client_port_no);
                    client_sender_addr.sin_addr.s_addr = htonl(INADDR_ANY); //INADDR_ANY, INADDR_LOOP

                    //bind
                    if(bind(client_sender_sd, (struct sockaddr*)&client_sender_addr,sizeof(client_sender_addr))<0)
                    {
                        perror("Client Receiver Socket: bind failed");
                        exit(-1);
                    }
                    //listen
                    if(listen(client_sender_sd,5)<0)
                    {
                        perror("Client Receiver Socket: listen failed");
                        close(client_sender_sd);
                        exit(-1);
                    }

                    //b.server connects to that port using port 20; client accepts connection
                    unsigned int server_data_len = sizeof(server_data_addr);
                    int server_data_sd = accept(client_sender_sd,(struct sockaddr *) &server_data_addr,&server_data_len);
                    printf("Client Port Accepting: %d\n",client_sender_addr.sin_port);
                    //5.after connection is established, send file data from client to server
                    

                    char line[256];
                    while (fgets(line, sizeof(line), fptr) != NULL) 
                    {
                        
                        if (send(server_data_sd, line, sizeof(line), 0) == -1) 
                        {
                            perror("Error Sending file..\n");
                            break;
                        }
                        memset(line, 0, sizeof(line));
                    }
                    
                    fclose(fptr);
                    //6.close connection
                    fflush(stdout);
                    close(server_data_sd);
                    close(client_sender_sd);

                    char server_message[256]; //to receive server's message 
                    bzero(server_message,sizeof(server_message));
                    recv(server_fd,server_message,sizeof(server_message),0); // receive message from server: 226 Transfer completed.
                    printf("%s\n",server_message);

                    
                }
                else{
                    bzero(&response,sizeof(response));
                    send(server_fd, new_command, strlen(new_command), 0);
                    recv(server_fd, response, sizeof(response), 0);
                    printf("%s\n", response);
                    // if(strcmp(response,"150 File status okay; about to open. data connection.") == 0) //error handling in case file is invalid
                    // {   //4.create a data connection by:
                    
    
                    //     //a.opening and listening to a port >1024 in the client
                        
                        
                        
                    // }
                    // else{
                    //     continue;
                    // }
                }
                //3.sending STOR command: Server responds with: 150 File status okay; about to open. data connection. or 530: Not Logged in.
                

                

            }

        }


        //RETR command - download
        else if(strncmp(command, "RETR",4) == 0 ||  strncmp(command, "retr",4) == 0)
        {
            //need to download file from current server directory to current client directory
            //things to do:
            




            //1.sending client port over to server using PORT command: Server responds with: 200 PORT command successful.
                // client_port_no++;
                int p1,p2;
                p1 = client_port_no/256;
                p2 = client_port_no%256;
                char port_command[256] = "PORT 127,0,0,1,";
                char p_1[256];
                char p_2[256];
                sprintf(p_1, "%d", p1);
                sprintf(p_2, "%d", p2);
                strcat(port_command,p_1);
                strcat(port_command,",");
                strcat(port_command,p_2);
                bzero(&response,sizeof(response));
                send(server_fd, port_command, strlen(port_command), 0);
                recv(server_fd, response, sizeof(response), 0);
                if(strcmp(response,"200 PORT command successful.") == 0)
                {
                    printf("%s\n", response);
                }

            //2.sending STOR command: Server responds with: 150 File status okay; about to open. data connection or 550 No such file or directory or 530: Not Logged in.
                
                char new_command[256];
                strcpy(new_command,command);
                char delim[] = " ";
                char *ptr = strtok(command,delim);
                ptr = strtok(NULL,delim);

                if(ptr != NULL)
                {
                    char filename[256];
                    strcpy(filename,ptr);
                    int pid = fork();
                    if (pid == 0)
                    {
                        int client_receiver_sd = socket(AF_INET,SOCK_STREAM,0);
                        if(client_receiver_sd<0)
                        {
                            perror("Client Receiver Socket creation:");
                            exit(-1);
                        }
                        //setsock
                        int value  = 1;
                        setsockopt(client_receiver_sd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)); //&(int){1},sizeof(int)
                        struct sockaddr_in client_receiver_addr, server_data_addr;

                        bzero(&client_receiver_addr,sizeof(client_receiver_addr));

                        client_receiver_addr.sin_family = AF_INET;
                        client_receiver_addr.sin_port = htons(client_port_no);
                        client_receiver_addr.sin_addr.s_addr = htonl(INADDR_ANY); //INADDR_ANY, INADDR_LOOP

                        //bind
                        if(bind(client_receiver_sd, (struct sockaddr*)&client_receiver_addr,sizeof(client_receiver_addr))<0)
                        {
                            perror("Client Receiver Socket: bind failed");
                            exit(-1);
                        }
                        //listen
                        printf("Client Port Listening: %d\n",htons(client_port_no));
                        if(listen(client_receiver_sd,5)<0)
                        {
                            perror("Client Receiver Socket: listen failed");
                            close(client_receiver_sd);
                            exit(-1);
                        }

                        //b.server connects to that port using port 20; client accepts connection
                        unsigned int server_data_len = sizeof(server_data_addr);
                        int server_data_sd = accept(client_receiver_sd,(struct sockaddr *) &server_data_addr,&server_data_len);
                        if(server_data_sd < 0){
                            perror("Accept error\n");
                        }
                        
                        printf("Client Port Accepting: %d\n",client_receiver_addr.sin_port);
                        
                        //5.after connection is established, send file data from server to client
                        char delim[] = " ";
                        char *file_name = strtok(new_command, delim);
                        file_name = strtok(NULL, delim);

                    
                        char client_file[50];

                        // add "Client-" to the file in case FTPserver and FTPclient are in same directory 
                        strcpy(client_file, "Client-");
                        strcat(client_file, file_name);
                        printf("this is the filname %s\n",file_name);
                        FILE *file;
                        // Create the file for writing 
                        if (!(file = fopen(client_file, "w")))
                        {
                            perror("Sorry, this file can't be created.");
                            return 0;
                        }
                        else
                        {
                            char server_data[256];

                            int server_return_value = 0;
                            // start receiving data from the server
                            memset(server_data, 0, sizeof(server_data));
                            while ((server_return_value = recv(server_data_sd, server_data, sizeof(server_data), 0)) > 0)
                            {
                                // write received data into the file 
                                fputs(server_data, file);
                                memset(server_data, 0, sizeof(server_data));
                            }

                            fclose(file);
                        }

                        fflush(stdout);
                        //6.close connection
                        close(server_data_sd);
                        close(client_receiver_sd);
                        
                        
                        char server_message[256]; //to receive server's message 
                        bzero(server_message,sizeof(server_message));
                        recv(server_fd,server_message,sizeof(server_message),0); // receive message from server: 226 Transfer completed.
                        printf("%s\n",server_message);
                        
                            
                    }
                    else
                    {
                        bzero(&response,sizeof(response));
                        send(server_fd, new_command, strlen(new_command), 0);
                        recv(server_fd, response, sizeof(response), 0);
                        printf("%s\n", response);
                        // if(strcmp(response,"150 File status okay; about to open. data connection.") == 0)
                        // {
                        
                        // }
                    }
                       
                }
                else
                {
                    bzero(&response,sizeof(response));
                    send(server_fd, new_command, strlen(new_command), 0);
                    recv(server_fd, response, sizeof(response), 0);
                    printf("%s\n", response);
                    
                }
                   

                    
                // else
                // {
                //     bzero(&response,sizeof(response));
                //     send(server_fd, new_command, strlen(new_command), 0);
                //     recv(server_fd, response, sizeof(response), 0);
                //     printf("%s\n", response);
                // }
                fflush(stdout);
               
            
            
        }


        //LIST command
        else if(strncmp(command, "LIST",4) == 0 ||  strncmp(command, "list",4) == 0){
            
            //1.sending client port over to server using PORT command: Server responds with: 200 PORT command successful.
                // client_port_no += 1;
                printf("client port: %d\n",client_port_no);
                int p1,p2;
                p1 = client_port_no/256;
                p2 = client_port_no%256;
                char port_command[256] = "PORT 127,0,0,1,";
                char p_1[256];
                char p_2[256];
                sprintf(p_1, "%d", p1);
                sprintf(p_2, "%d", p2);
                strcat(port_command,p_1);
                strcat(port_command,",");
                strcat(port_command,p_2);
                bzero(&response,sizeof(response));
                send(server_fd, port_command, strlen(port_command), 0);
                recv(server_fd, response, sizeof(response), 0);
                if(strcmp(response,"200 PORT command successful.") == 0)
                {
                    printf("%s\n", response);
                }
                int client_receiver_sd = socket(AF_INET,SOCK_STREAM,0);
                        if(client_receiver_sd<0)
                        {
                            perror("Client Receiver Socket creation:");
                            exit(-1);
                        }
                        //setsock
                        // int value  = 1;
                        // setsockopt(client_receiver_sd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)); //&(int){1},sizeof(int)
                        // struct sockaddr_in client_receiver_addr;

                        // bzero(&client_receiver_addr,sizeof(client_receiver_addr));

                        // client_receiver_addr.sin_family = AF_INET;
                        // client_receiver_addr.sin_port = htons(client_port_no);
                        // client_receiver_addr.sin_addr.s_addr = htonl(INADDR_ANY); //INADDR_ANY, INADDR_LOOP

                        struct sockaddr_in client_receiver_addr;	//structure to save IP address and port
                        memset(&client_receiver_addr,0,sizeof(client_receiver_addr)); //Initialize/fill the server_address to 0
                        client_receiver_addr.sin_family = AF_INET;	//address family
                        client_receiver_addr.sin_port = htons(client_port_no);	//port
                        client_receiver_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //htonl(INADDR_LOOPBACK); //inet_addr("127.0.0.1");

                        setsockopt(client_receiver_sd,SOL_SOCKET,SO_REUSEADDR,&(int){1},sizeof(int)); //&(int){1},sizeof(int)

                        //bind
                        if(bind(client_receiver_sd, (struct sockaddr*)&client_receiver_addr,sizeof(client_receiver_addr))<0)
                        {
                            perror("Client Receiver Socket: bind failed");
                            exit(-1);
                        }
                        //listen
                        printf("Client Port Listening: %d\n",htons(client_port_no));
                        if(listen(client_receiver_sd,5)<0)
                        {
                            perror("Client Receiver Socket: listen failed");
                            close(client_receiver_sd);
                            exit(-1);
                        }

                // printf("If is working\n");
                    int pid = fork();
                    if(pid == 0)
                    {

                        
                        
                        //b.server connects to that port using port 20; client accepts connection
                        printf("Client Port Accepting: %d\n",client_receiver_addr.sin_port);
                        // print_it = 0;
                        int server_data_sd = accept(client_receiver_sd,NULL,NULL);
                        if(server_data_sd <1){
                            perror("Client Data Accept Error.");
                            exit(-1);
                        }
                        
                        printf("Client time %lu \n",time(NULL));
                       
                        //c.server sends data to client
                        // printf("Client Port After Accepting: %d\n",htons(client_port_no));
                        char server_message[256]; //to receive server's message 
                        bzero(server_message,sizeof(server_message));
                        while(recv(server_data_sd,server_message,sizeof(server_message),0) != 0)
                        { // receive message from server: <list of files> 226 Transfer completed.
                            printf("%s\n",server_message);
                            // bzero(server_message,sizeof(server_message));
                            fflush(stdout);
                        }
                        
                        close(server_data_sd);
                        close(client_receiver_sd);
                        // return NULL;
                        // print_it = 1;
                        //4.close connection
                        bzero(&response,sizeof(response));
                        recv(server_fd, response, sizeof(response), 0);
                        printf("%s\n", response);
                        
                        
                    }
                    else
                    {
                        bzero(&response,sizeof(response));
                        send(server_fd, command, strlen(command), 0);
                        recv(server_fd, response, sizeof(response), 0);
                        printf("%s\n", response);
                        close(client_receiver_sd);
                       
                        // wait(NULL);
                    }
                    


            //2.sending LIST command: Server responds with: 150 File status okay; about to open. data connection.
                
              

            //3.creating data connection
                //a.opening and listening to a port >1024 in the client
                // printf("response: %s %lu\n",response, strlen(response));
                // printf("150 File status okay; about to open data connection.: %lu\n", strlen("150 File status okay; about to open data connection."));
                // if(strcmp(response,"150 File status okay; about to open data connection.") == 0)
                // {
                    
                // }
            
        }


        //!LIST command
        else if(strncmp(command, "!LIST",5) == 0 ||  strncmp(command, "!list",5) == 0){
            system("ls");
        }


        //CWD command
        else if(strncmp(command, "CWD",3) == 0 ||  strncmp(command, "cwd",3) == 0){
            send(server_fd, command, strlen(command), 0);
            bzero(&response,sizeof(response));
            recv(server_fd, response, sizeof(response), 0);
            // char string_terminate = '\0';
            // strncat(response,&string_terminate,1);
            fflush(stdout);
            printf("%s\n", response);
        }



        //!CWD command
        else if(strncmp(command, "!CWD",4) == 0 ||  strncmp(command, "!cwd",4) == 0){
            
            char delim[] = " ";
            char *foldername = strtok(command,delim);
            foldername = strtok(NULL,delim);

            
            printf("Changing to folder %s\n", foldername);
            if (chdir(foldername) != 0){
                printf("Change to %s failed\n", foldername);
            } 

        }



        //PWD command
        else if(strncmp(command, "PWD",3) == 0 ||  strncmp(command, "pwd",3) == 0){
            send(server_fd, command, strlen(command), 0);
            bzero(&response,sizeof(response));
            recv(server_fd, response, sizeof(response), 0);
            fflush(stdout);
            printf("%s\n", response);
        }


        //!PWD command
        else if(strncmp(command, "!PWD",4) == 0 ||  strncmp(command, "!pwd",4) == 0){
            
            system("pwd");

        }


        //invalid command
        else
        {
            char invalid[256];
            strcpy(invalid, "INVALID");
            bzero(&response,sizeof(response));
            send(server_fd, invalid, strlen(invalid), 0);
            recv(server_fd, response, sizeof(response), 0);
            printf("%s\n", response);
        }
    }




    return 0;
}

