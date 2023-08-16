default: bwtdecode bwtsearch

bwtdecode: bwtdecode.c helper.h
	gcc -lm -Wall -Werror bwtdecode.c helper.h -o bwtdecode   
bwtsearch: bwtsearch.c helper2.h
	gcc -lm -Wall -Werror bwtsearch.c helper2.h -o bwtsearch

 	

