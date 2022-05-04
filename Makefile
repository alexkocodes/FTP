output: client.c server.c server_2.c
	gcc client.c -o client.out && gcc server.c -o server.out && gcc server_2.c -o server_2.out

clean: 
	rm *.out