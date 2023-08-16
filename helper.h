#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int FILELEN;
int bufferStart;
int bucketBuffer;
int totalBuckets;
const int SMALLFILE      = 3000000;
int BUFFERSIZE     = 819200;
const int BUCKETINTERVAL = 128;
const int CHARTYPES = 5; 
size_t MAXQUERY = 100;
int BLOCKSIZE      = 128;


FILE* outfile; 
FILE* infile;
int indes;
int* cMap;
int* rank; //occurrences
int** occBucket;
char* buffer;
char* query;
int occ(char c, int pos,int code);
int constructTablesL(void);
int constructTablesS(char* code);
void  decodeInputL(int startIdx);
void  decodeInputS(int startIdx,char* code);
void seekBucket(int b);
void initializeGlobal(void);
void cleanupGlobal(void);
int getCharCode(char c);


