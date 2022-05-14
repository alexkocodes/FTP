//FTP Client
#include <signal.h>
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
#define MAX_PORTS 20


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
    //connecting to server socket
    if(connect(server_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        perror("Connection Failed.");
        exit(-1);
    }
   

    socklen_t len = sizeof(server_addr);
    if (getsockname(server_fd, (struct sockaddr *)&server_addr, &len) == -1)
    {
        perror("Error in getsockname");
        return 0;
    }

    //getting client port number
    
    int client_port_no = ntohs(server_addr.sin_port);
    int initial_port = client_port_no;
    int fork_pid_array[100] = {0};
    int fork_counter = 0; 
   

    //Beginning of FTP User Interface

    //facilitating command response interaction between client and server
    char command[256];
    char response[512];
    char client_input[256];
    int l = 1;
    while(l)
    {
        //client data ports are reused after 20 ports
        if(client_port_no >= MAX_PORTS+initial_port){
            client_port_no = initial_port;
        }
        bzero(command,sizeof(command));
        bzero(&response,sizeof(response));
        
       
        
        printf("\nftp> ");
        fgets(command, sizeof(command), stdin); 
        
        command[strcspn(command, "\n")] = 0; //removing trailing \n in string
       
        strcpy(client_input,command);
        char delim[] = " ";
        char *ctr = strtok(command,delim); //getting just the command from the input ie user from user bob
        
        

        //QUIT command 
        if (strcmp(command, "QUIT") == 0 ||  strcmp(command, "quit") == 0){
            if(send(server_fd, command, strlen(command), 0) < 0){
                perror("Error in send.");
                exit(-1);
            };
            bzero(&response,sizeof(response));
            recv(server_fd, response, sizeof(response), 0);
            
            printf("%s\n", response);
            close(server_fd);
            // in order to kill all processes for effective quitting
            // if(fork_pid_array != 0){
                
                for(int i =0; i < fork_counter; i++){
                    int rd = kill(fork_pid_array[fork_counter],SIGINT);
                    
                }
            // }
            
            return 0;
        }


        //USER command
        else if(strcmp(command, "USER") == 0 ||  strcmp(command, "user") == 0){
            
            
            if(send(server_fd, client_input, strlen(client_input), 0) < 0){
                perror("Error in send.");
                exit(-1);
            }
            bzero(&response,sizeof(response));
            recv(server_fd, response, sizeof(response), 0);
            printf("%s\n", response);
            fflush(stdout);
        }


        //PASS command
        else if(strcmp(command, "PASS") == 0 ||  strcmp(command, "pass") == 0){
            bzero(&response,sizeof(response));
            send(server_fd, client_input, strlen(client_input), 0);
            recv(server_fd, response, sizeof(response), 0);
            printf("%s\n", response);
        }


        //STOR command - upload
        else if(strcmp(command, "STOR") == 0 ||  strcmp(command, "stor") == 0){

            //need to upload file from current client directory to current server directory
            //things to do:

            //update port
            client_port_no++;
            //1.send port command
            char new_command[256];
            strcpy(new_command,client_input);
            char delim[] = " ";
            char *ptr = strtok(new_command,delim);
            ptr = strtok(NULL,delim);
            int p1,p2;
            p1 = client_port_no/256;
            p2 = client_port_no%256;
            char port_command[256] = "PORT 127,0,0,1,";
            char port_response[256];
            char p_1[256];
            char p_2[256];
            sprintf(p_1, "%d", p1);
            sprintf(p_2, "%d", p2);
            strcat(port_command,p_1);
            strcat(port_command,",");
            strcat(port_command,p_2);
            bzero(&response,sizeof(response));
            bzero(&port_response,sizeof(port_response));
            send(server_fd, port_command, strlen(port_command), 0);
            recv(server_fd, port_response, sizeof(port_response), 0);
            if(strcmp(port_response,"200 PORT command successful.") == 0)
            {
                printf("%s\n", port_response);
                if(ptr!= NULL) //checking if filename is given in input
                {
                    //2.see if file exists
                    char filename[256];
                    strcpy(filename,ptr);
                    FILE *fptr = fopen(filename,"r");		//open requested file
                    if(fptr == NULL) //serving error message in case file not present in disk
                    {	
                        printf("550 No such file or directory.\n");
                        continue;
                    }
                    
                    else
                    {

                        

                        //3.building data channel to send data to server
                        int pid = fork(); //forking for concurrent relationship in the same client
                        if(pid == 0)
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
                            // bzero(&response,sizeof(response));
                            // send(server_fd, client_input, strlen(client_input), 0);
                            // recv(server_fd, response, sizeof(response), 0);
                            // printf("%s\n", response);
                            //b.server connects to that port using port 20; client accepts connection
                            unsigned int server_data_len = sizeof(server_data_addr);
                            int server_data_sd = accept(client_sender_sd,(struct sockaddr *) &server_data_addr,&server_data_len);
                            
                            //4.after connection is established, send file data from client to server
                            
                        

                            while(1)
                            {
                                /* First read file in chunks of 256 bytes */
                                unsigned char buff[1024]={0};
                                int nread = fread(buff,1,1024,fptr);
                                

                                /* If read was success, send data. */
                                if(nread > 0)
                                {
                                    
                                    write(server_data_sd, buff, nread);
                                }
                                if (nread < 1024)
                                {
                                    if (feof(fptr))
                                    {
                                        
                                    }
                                    if (ferror(fptr))
                                        printf("Error reading\n");
                                    break;
                                }
                            }
                            fclose(fptr);
                            //5.close connection
                            fflush(stdout);
                            close(server_data_sd);
                            close(client_sender_sd);

                            char server_message[256]; //to receive server's message 
                            bzero(server_message,sizeof(server_message));
                            recv(server_fd,server_message,sizeof(server_message),0); // receive message from server: 226 Transfer completed.
                            printf("\r%s\n",server_message);
                            printf("\nftp> ");
                            exit(0);

                            return 0;
                        }
                        else
                        {
                            bzero(&response,sizeof(response));
                            send(server_fd, client_input, strlen(client_input), 0);
                            recv(server_fd, response, sizeof(response), 0);
                            printf("%s\n", response);
                            fork_pid_array[fork_counter] = pid;
                            fork_counter++;
                            
                        }

                    }
                }
                else{
                    bzero(&response,sizeof(response));
                    send(server_fd, client_input, strlen(client_input), 0);
                    recv(server_fd, response, sizeof(response), 0);
                    printf("%s\n",response);
                }
            }
            else{
                 //in case we do not receive 200 Port command successful
                printf("%s\n", port_response);
            }

        }


        //RETR command - download
        else if(strcmp(command, "RETR") == 0 ||  strcmp(command, "retr") == 0)
        {
            //need to download file from current server directory to current client directory
            //things to do:
            


            //update client port
            client_port_no++;

            //1.sending client port over to server using PORT command: Server responds with: 200 PORT command successful.
               
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
            

        //2.sending RETR command: Server responds with: 150 File status okay; about to open. data connection or 550 No such file or directory or 530: Not Logged in.
            
                char new_command[256];
                strcpy(new_command,client_input);
                char delim[] = " ";
                char *ptr = strtok(new_command,delim);
                ptr = strtok(NULL,delim);

                if(ptr != NULL)
                {
                    char filename[256];
                    strcpy(filename,ptr);
                    int pid = fork(); //forking for concurrent relationship in the same client
                    if (pid == 0)
                    {
                        //3.building the data channel
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
                        
                        if(listen(client_receiver_sd,5)<0)
                        {
                            perror("Client Receiver Socket: listen failed");
                            close(client_receiver_sd);
                            exit(-1);
                        }
                        
                        printf("%s\n", response);
                        //b.server connects to that port using port 20; client accepts connection
                        unsigned int server_data_len = sizeof(server_data_addr);
                        int server_data_sd = accept(client_receiver_sd,(struct sockaddr *) &server_data_addr,&server_data_len);
                        if(server_data_sd < 0){
                            perror("Accept error\n");
                        }
                        
                        
                        
                        //4.after connection is established, send file data from server to client
                        // char delim[] = " ";
                        char file_name[256];
                        strcpy(file_name,ptr);

                    
                        char client_file[50];

                        // add "Client-" to the file to assure that we received file in client
                        strcpy(client_file, "Client-");
                        strcat(client_file, file_name);
                        
                        //5. store data sent by server in the file
                        FILE *file;
                        

                        
                        int bytesReceived = 0;
                        char recvBuff[1024];
                        memset(recvBuff, '0', sizeof(recvBuff));
                        file = fopen(client_file, "w"); 
                        
                        if(NULL == file)
                        {
                        printf("Error opening file");
                        return 1;
                        }
                        
                        while((bytesReceived = read(server_data_sd, recvBuff, 1024)) > 0)
                        { 
                        
                            
                            fflush(stdout);
                            fwrite(recvBuff, 1,bytesReceived,file);
                            
                        }

                        if(bytesReceived < 0)
                        {
                            printf("\n Read Error \n");
                        }
                        
                        fclose(file);
                        
                        //6.close connection
                        close(server_data_sd);
                        close(client_receiver_sd);
                        
                        
                        char server_message[256]; //to receive server's message 
                        bzero(server_message,sizeof(server_message));
                        recv(server_fd,server_message,sizeof(server_message),0); // receive message from server: 226 Transfer completed.
                        printf("\r%s\n",server_message);
                        printf("\nftp> ");
                        exit(0);
                        return 0;

                    
                    } 
                    else
                    {
                        bzero(&response,sizeof(response));
                        send(server_fd, client_input, strlen(client_input), 0);
                        recv(server_fd, response, sizeof(response), 0);
                        printf("%s\n", response);
                        fork_pid_array[fork_counter] = pid;
                        fork_counter++;
                        
                    }
            

                }   
                else
                {
                    bzero(&response,sizeof(response));
                    send(server_fd, new_command, strlen(new_command), 0);
                    recv(server_fd, response, sizeof(response), 0);
                    printf("%s\n", response);
                }
                fflush(stdout);
            
            }
            else
            {
                //in case we do not receive 200 Port command successful
                printf("%s\n", response);
            }
            
        }


        //LIST command
        else if(strcmp(command, "LIST") == 0 ||  strcmp(command, "list") == 0)
        {
            
            //1.sending client port over to server using PORT command: Server responds with: 200 PORT command successful.
                //update client port
                client_port_no++;
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
                    
                    //2.building data channel
                    int client_receiver_sd = socket(AF_INET,SOCK_STREAM,0);
                    if(client_receiver_sd<0)
                    {
                        perror("Client Receiver Socket creation:");
                        exit(-1);
                    }
                   

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
                    
                    if(listen(client_receiver_sd,5)<0)
                    {
                        perror("Client Receiver Socket: listen failed");
                        close(client_receiver_sd);
                        exit(-1);
                    }

                    
                            
                            
                            //3.server connects to that port using port 20; client accepts connection
                            int pid = fork();
                            if(pid == 0)
                            {
                                
                            
                                int server_data_sd = accept(client_receiver_sd,NULL,NULL);
                                if(server_data_sd <1){
                                    perror("Client Data Accept Error.");
                                    exit(-1);
                                }
                                
                                
                            
                                //4.server sends data to client
                                
                                char server_message[256]; //to receive server's message 
                                bzero(server_message,sizeof(server_message));
                                printf("\r");
                                while(recv(server_data_sd,server_message,sizeof(server_message),0) != 0)
                                { // receive message from server: <list of files> 226 Transfer completed.
                                    printf("%s\n",server_message);
                                
                                    fflush(stdout);
                                }
                                
                            
                                
                                //4.close connection
                                bzero(&response,sizeof(response));
                                recv(server_fd, response, sizeof(response), 0);
                                printf("%s\n", response);
                               
                                close(server_data_sd);
                                close(client_receiver_sd);
                                fflush(stdout);
                                
                                printf("\nftp> ");
                                exit(0);
                                
                                return 0;
                            }   
                            else
                            {   
                                
                                bzero(&response,sizeof(response));
                                send(server_fd, command, strlen(command), 0);
                                recv(server_fd, response, sizeof(response), 0);
                                printf("%s\n", response);
                                close(client_receiver_sd);
                                fork_pid_array[fork_counter] = pid;
                                fork_counter++;
                                // continue;
                                // return NULL;
                                // l = 1; 
                                // break;
                                
                            }
                            
                    
                }  
                else{
                    printf("%s\n", response);
                }


            
            
        }


        //!LIST command
        else if(strcmp(command, "!LIST") == 0 ||  strcmp(command, "!list") == 0)
        {
           
            system("ls");
            fflush(stdout);
        }


        //CWD command
        else if(strcmp(command, "CWD") == 0 ||  strcmp(command, "cwd") == 0)
        {
            send(server_fd, client_input, strlen(client_input), 0);
            bzero(&response,sizeof(response));
            recv(server_fd, response, sizeof(response), 0);
            
            fflush(stdout);
            printf("%s\n", response);
        }



        //!CWD command
        else if(strcmp(command, "!CWD") == 0 ||  strcmp(command, "!cwd") == 0)
        {
            
            char delim[] = " ";
            char *foldername = strtok(client_input,delim);
            foldername = strtok(NULL,delim);
            if(foldername == NULL)
            {
                printf("No such file or directory.\n");
            }

            else
            {
                printf("Changing to local folder %s\n", foldername);
                if (chdir(foldername) != 0)
                {
                    printf("Change to %s failed\n", foldername);
                    printf("No such file or directory.\n");
                } 
                fflush(stdout);
            }
            

        }



        //PWD command
        else if(strcmp(command, "PWD") == 0 ||  strcmp(command, "pwd") == 0)
        {
            send(server_fd, command, strlen(command), 0);
            bzero(&response,sizeof(response));
            recv(server_fd, response, sizeof(response), 0);
            fflush(stdout);
            printf("%s\n", response);
        }


        //!PWD command
        else if(strcmp(command, "!PWD") == 0 ||  strcmp(command, "!pwd") == 0)
        {
            
            system("pwd");
            fflush(stdout);

        }


        //invalid command
        else
        {
            char invalid[256];
            
            strcpy(invalid, "invalid");
            bzero(&response,sizeof(response));
            send(server_fd, invalid, strlen(invalid), 0);
            recv(server_fd, response, sizeof(response), 0);
            printf("%s\n", response);
        }
        
    }



    
    return 0;
}

