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

void *thread_function(void *arg){

	return NULL;

}
int main()
{

	//1. socket
	int server_sd = socket(AF_INET,SOCK_STREAM,0);
	if(server_sd<0)
	{
		perror("Socket Failed: ");
		return -1;
	}

	//2. bind
	struct sockaddr_in server_address;	//structure to save IP address and port
	memset(&server_address,0,sizeof(server_address)); //Initialize/fill the server_address to 0
	server_address.sin_family = AF_INET;	//address family
	server_address.sin_port = htons(80);	//port
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); //htonl(INADDR_LOOPBACK); //inet_addr("127.0.0.1");

	setsockopt(server_sd,SOL_SOCKET,SO_REUSEADDR,&(int){1},sizeof(int)); //&(int){1},sizeof(int)


	if(bind(server_sd,(struct sockaddr *)&server_address,sizeof(server_address))<0) 
	{
		perror("Bind failed..:");
		return -1;
	}

	//3. listen
	if(listen(server_sd,5)<0)
	{
		perror("Listen Error:");
		return -1;
	}

	

	struct sockaddr_in client_address;

	pthread_t array[4];
	int i = 0;
	
	//4. accept
	while(1)
	{

		
		int client_sd = accept(server_sd,NULL,NULL);
		//int client_len=sizeof(client_address);
		//int client_sd = accept(server_sd,(struct sockaddr*)&client_address,(socklen_t *)(&client_len));
		if(client_sd<1)
		{
			perror("Accept Error:");
			return -1;
		}
		else
		{
			
		}
	}	

	close(server_sd);										//close the master/server socket
	//6. close
	return 0;
}