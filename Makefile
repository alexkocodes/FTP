output: client/c_client.c server/s_server.c
	gcc client/c_client.c -o client/c_client.out && gcc server/s_server.c -o server/s_server.out

clean: 
	find . -name \*.out -type f -delete