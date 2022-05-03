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
    else if (strlen(argv[1] > 15)){
        printf("Length of IP Address is invalid. Should be 15 characters or less.");
        return 0;
    }
    else if (strlen(argv[2] > 5)){
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

    //connecting to server socket
    if(connect(server_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        perror("Connection Failed.");
        exit(-1);
    }

    printf("Connected to server with IP: %s and Port: %d",server_ip,server_port);


    //Beginning of FTP User Interface

    //facilitating command response interaction between client and server
    char command[256];
    char response[256];


    while(1){
        bzero(command,sizeof(command));

        printf("ftp> ");
        fgets(command, sizeof(command), stdin); 
        command[strcspn(command, "\n")] = 0; //removing trailing \n in string

        //QUIT command 
        if (strncmp(command, "QUIT",4) == 0 ||  strncmp(command, "quit",4) == 0){
            send(server_fd, command, sizeof(command), 0);
            close(server_fd);
            break;
        }


        //USER command
        else if(strncmp(command, "USER",4) == 0 ||  strncmp(command, "user",4) == 0){
            send(server_fd, command, sizeof(command), 0);
            recv(server_fd, response, sizeof(response), 0);
            printf("%s\n", response);
        }


        //PASS command
        else if(strncmp(command, "PASS",4) == 0 ||  strncmp(command, "pass",4) == 0){
            send(server_fd, command, sizeof(command), 0);
            recv(server_fd, response, sizeof(response), 0);
            printf("%s\n", response);
        }


        //STOR command - upload
        else if(strncmp(command, "STOR",4) == 0 ||  strncmp(command, "stor",4) == 0){

        }


        //RETR command - download
        else if(strncmp(command, "RETR",4) == 0 ||  strncmp(command, "retr",4) == 0){

        }


        //LIST command
        else if(strncmp(command, "LIST",4) == 0 ||  strncmp(command, "list",4) == 0){
            send(server_fd, command, sizeof(command), 0);
            recv(server_fd, response, sizeof(response), 0);
            printf("%s\n", response);
        }


        //!LIST command
        else if(strncmp(command, "!LIST",5) == 0 ||  strncmp(command, "!list",5) == 0){
            system("ls");
        }


        //CWD command
        else if(strncmp(command, "CWD",3) == 0 ||  strncmp(command, "cwd",3) == 0){
            send(server_fd, command, sizeof(command), 0);
            recv(server_fd, response, sizeof(response), 0);
            printf("%s\n", response);
        }



        //!CWD command
        else if(strncmp(command, "!CWD",4) == 0 ||  strncmp(command, "!cwd",4) == 0){
            
            char delim[] = " ";
            char *foldername = strtok(command,delim);
            foldername = strtok(NULL,delim);

            
            printf("Changing to folder %s and executing %s\n", foldername);
            if (chdir(foldername) != 0){
                printf("Change to %s failed\n", foldername);
            } 

        }



        //PWD command
        else if(strncmp(command, "PWD",3) == 0 ||  strncmp(command, "pwd",3) == 0){
            send(server_fd, command, sizeof(command), 0);
            recv(server_fd, response, sizeof(response), 0);
            printf("%s\n", response);
        }


        //!PWD command
        else if(strncmp(command, "!PWD",4) == 0 ||  strncmp(command, "!pwd",4) == 0){
            
            system("pwd");

        }


        //invalid command
        else{
            char invalid[256];
            strcpy(invalid, "INVALID");
            send(server_fd, invalid, sizeof(invalid), 0);
            recv(server_fd, response, sizeof(response), 0);
            printf("%s\n", response);
        }
    }




    return 0;
}

