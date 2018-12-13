sortermake:
	gcc -pthread -o server serverClient.c
	gcc -pthread -o client bankingClient.c
testclient:
		./client ilab.cs.rutgers.edu 8701
testserver:
		./server 8701
