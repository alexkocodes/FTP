output: client.c server.c server_2.c client/c_client.c server/s_server.c
	gcc client.c -o client.out && gcc server.c -o server.out && gcc server_2.c -o server_2.out && gcc client/c_client.c -o client/c_client.out && gcc server/s_server.c -o server/s_server.out

clean: 
	find . -name \*.out -type f -delete