sortermake:
	gcc -pthread -o server serverClientfin.c
	gcc -pthread -o client bankingClientfin.c
testclient:
		./client ilab.cs.rutgers.edu 8701
testserver:
		./server 8701
