#include "helper2.h"

void cleanupGlobal(){
        for (int i =0 ; i< totalBuckets; ++i){
            free(occBucket[i]);
        }
        free(occBucket);
    free(cMap);
    free(query);
    free(buffer);
}

void initializeGlobal(){
    fseek(infile, 0, SEEK_END);
    FILELEN = ftell(infile);
    fseek(infile,0,SEEK_SET);
    query = malloc(MAXQUERY*sizeof(char));
    //bleb
    if (FILELEN < BUFFERSIZE)
        BUFFERSIZE = FILELEN;
    if (FILELEN < BLOCKSIZE)
        BLOCKSIZE = FILELEN; 
    buffer = malloc(BUFFERSIZE * sizeof(char));
    totalBuckets = FILELEN/BUCKETINTERVAL;
    occBucket = malloc(sizeof(int*)*totalBuckets);
    for (int i = 0; i<totalBuckets; ++i){
        occBucket[i] = malloc(sizeof(int)*4);
    }
    cMap = malloc(sizeof(int)*5);
}


int getCharCode(char c){
    switch(c) {
        case '\n':
            return 0;
        case 'A':
            return 1;
        case 'C':
            return 2;
        case 'G':
            return 3;
        case 'T':
            return 4;
        default:
            return 5;
   }
}

int occ(char c, int pos, int code){
    if (c == '\n')
        return 0;
    int count = 0;
    int b = pos/BUCKETINTERVAL;

    int start = b*BUCKETINTERVAL - bufferStart;
    int end = pos - bufferStart;
    for(int i =start;i <= end; ++i){
        if(buffer[i] == c){
            ++count;
        }
    }
    if (b == 0)
        return count;
    return count + occBucket[b-1][code-1];
}

void seekBuffer(int b,int pos){
    bufferStart = b*BUCKETINTERVAL;
    if (bufferStart < 0)
        bufferStart = 0;
    if(bufferStart + BLOCKSIZE >FILELEN){
        bufferStart = FILELEN-BLOCKSIZE;
    }
    pread(indes, buffer, BLOCKSIZE, bufferStart);
}

int bwtSearch(int length){
    int i   = length-1;
    char c  = query[i];
    
    int charCode = getCharCode(c);
    int First = cMap[charCode];
    int Last = 0;
    if (charCode == 4){
        Last  = cMap[0]-1;
    } else {
        Last  = cMap[charCode+1]-1;
    }
    if (Last <= 0) 
        Last = FILELEN-1;
    seekBuffer(First/BUCKETINTERVAL,First);
    while (First <= Last &&  i >= 1){
        c = query[i-1];
        charCode = getCharCode(c);
        if(First < bufferStart || First >= bufferStart + BLOCKSIZE)
            seekBuffer(First/BUCKETINTERVAL,First);
        int fOcc =  occ(c,First-1,charCode);
        First = cMap[charCode] +fOcc;
        if(Last<bufferStart || Last>=bufferStart+BLOCKSIZE)
            seekBuffer(Last/BUCKETINTERVAL,Last);
        int lOcc = occ(c,Last,charCode)-1;
        Last = cMap[charCode] + lOcc;
        --i;
    }
    if (Last<First){
        return 0;
    } else {
        return Last-First+1;
    }
}

void constructTables(void){
    // find file length;
    int charCode;
    bufferStart = 0;
    pread(indes, buffer, BUFFERSIZE, bufferStart);
    for (int i = 0; i < totalBuckets; ++i){
        // if buffer not enough to read to end
        if (bufferStart+BUFFERSIZE < (i+1) * BUCKETINTERVAL){
            bufferStart = i*BUCKETINTERVAL;
            pread(indes, buffer, BUFFERSIZE, bufferStart);
        }
        int bucketStart = i*BUCKETINTERVAL-bufferStart;
        for(int j=bucketStart; j<bucketStart+BUCKETINTERVAL; ++j){
            charCode = getCharCode(buffer[j]);
            ++cMap[charCode];
        }
        occBucket[i][0] = cMap[1];
        occBucket[i][1] = cMap[2];
        occBucket[i][2] = cMap[3];
        occBucket[i][3] = cMap[4];
    } 
    
    if( totalBuckets * BUCKETINTERVAL < FILELEN){
        if (bufferStart+BUFFERSIZE < FILELEN){
            bufferStart = totalBuckets*BUCKETINTERVAL;
            pread(indes, buffer, BUFFERSIZE, bufferStart);
        }
        int start =  totalBuckets*BUCKETINTERVAL-bufferStart;
        for(int i = start; i< FILELEN-bufferStart; ++i){
            charCode = getCharCode(buffer[i]);
            ++cMap[charCode];
        }
    }

    // modify cMap to count values before character
    int tmp = 0;
    int runningCount = 0;
    for(int i = 0; i<CHARTYPES; ++i){
        tmp = cMap[i];
        cMap[i] = runningCount;
        runningCount+=tmp;
    }
    
    free(buffer);
    buffer = malloc(BLOCKSIZE * sizeof(char));
    setvbuf (infile , buffer , _IOFBF , BLOCKSIZE );
}


int main (int argc, char* argv[]) {
    indes = open(argv[1],O_RDONLY);
    infile = fopen(argv[1],"r");

    if(infile == NULL || indes == -1){
        printf("Can't open file\n");
        exit(0); 
    }
    initializeGlobal();
    int length = 0;
    // find file length
    constructTables();
    while((length = getline(&query, &MAXQUERY,stdin)) != -1){
        printf("%d\n",bwtSearch(length-1));
    }
    
     
    fclose(infile);
    close(indes);
    cleanupGlobal();
    return 0;
}
