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

#define MAX_SOCKET_NUM 1024
#define MAX_USER 2

//a function to handle client connections that connect to server -> parameter: client descriptor and user array
// void handle_connection(int client_sd, struct user *user_array);

//a function to close client connections that connect to server -> parameter: client descriptor, user array and current set of file descriptors
// void close_client_connection(int client_sd, struct user *user_array, fd_set *currentfd);
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