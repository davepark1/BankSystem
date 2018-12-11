sortermake:
	gcc -pthread -o server server.c
	gcc -pthread -o client client.c
testclient:
		./client ilab.cs.rutgers.edu 8701
testserver:
		./server 8701
