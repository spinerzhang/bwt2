#include "helper.h"

void cleanupGlobal(){
    if (FILELEN < SMALLFILE+1){
        free(rank);
    }else{
        for (int i =0 ; i< totalBuckets; ++i){
            free(occBucket[i]);
        }
        free(occBucket);
    }
    free(cMap);
    free(buffer);
}

void initializeGlobal(){
    fseek(infile, 0, SEEK_END);
    FILELEN = ftell(infile);
    fseek(infile,0,SEEK_SET);
    //bleb
    if(FILELEN < SMALLFILE+1){
        buffer = malloc(FILELEN * sizeof(char));
        rank = malloc(sizeof(int)*FILELEN);
    }else {
        buffer = malloc(BUFFERSIZE * sizeof(char));
        rank = malloc(sizeof(int)*BUFFERSIZE);

        totalBuckets = FILELEN/BUCKETINTERVAL;
        occBucket = malloc(sizeof(int*)*totalBuckets);
        for (int i = 0; i<totalBuckets; ++i){
            occBucket[i] = malloc(sizeof(int)*4);
        }
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


int constructTablesL(void){
    // find file length;
    int startPos = 0;
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
            if (buffer[j] == '\n') startPos = j+bufferStart;
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
            if (buffer[i] == '\n'){
                startPos = i+bufferStart;
            }
            ++cMap[charCode];
        }
    }

    // modify cMap
    int tmp = 0;
    int runningCount = 0;
    for(int i = 0; i<CHARTYPES; ++i){
        tmp = cMap[i];
        cMap[i] = runningCount;
        runningCount+=tmp;
    }
    
    free(buffer);
    if (FILELEN / BUCKETINTERVAL < 60000){
        BLOCKSIZE = BUCKETINTERVAL * 5000;
    }else{
        BLOCKSIZE = BUCKETINTERVAL;
    }
    buffer = malloc(BLOCKSIZE * sizeof(char));
    setvbuf (infile , buffer , _IOFBF , BLOCKSIZE );
    return startPos;
}


int constructTablesS(char* code){
    int startPos = 0;
    for (int i = 0; i< FILELEN;++i){
        if (code[i]=='\n') startPos = i;
        rank[i] = cMap[getCharCode(code[i])];
        ++cMap[getCharCode(code[i])];     
    }
    //modify cMap to the less than values
    int runningValue = 0;
    int tmp = 0;
    for (int i = 0; i < CHARTYPES; ++i){
        if (i == 0){ // newline char
            cMap[0] = 0;
            ++runningValue;
            continue;
        }
        tmp = cMap[i];
        cMap[i] = runningValue;
        runningValue += tmp;
    }

    return startPos;
}

int occ(char c, int pos, int code){
    if (c == '\n')
        return 0;
    int count = 0;
    int b = pos/BUCKETINTERVAL;
    int start = b*BUCKETINTERVAL - bufferStart;
    int end = pos - bufferStart;
    for(int i =start ;i < end; ++i){
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


void decodeInputS(int startIdx, char* code){
    char curVal = '\n';
    int valIdx = startIdx;
    char decoded[FILELEN];
    for(int i = FILELEN-1; i>= 0; --i){
        decoded[i] = curVal;
        valIdx = rank[valIdx]+cMap[getCharCode(curVal)];
        curVal = code[valIdx];
    }
    fseek(outfile,0,SEEK_SET);
    fputs(decoded,outfile);
}

void decodeInputL(int startIdx){

    char curVal = '\n';
    int valIdx = startIdx;
    char decoded[BUFFERSIZE];
    int dIdx = BUFFERSIZE; //decoded index
    int remChar = FILELEN;
    int code = -1; 
    for(int i = FILELEN-1; i>= 0; --i){
        --dIdx;
        decoded[dIdx] = curVal;
        if (dIdx == 0) {
            fseek(outfile,i,SEEK_SET); 
            fwrite(decoded,1,BUFFERSIZE,outfile);
            remChar -= BUFFERSIZE;
            dIdx = BUFFERSIZE;
        }
        switch(curVal) {
            case '\n': code = 0; break;
            case 'A':  code = 1; break;
            case 'C':  code = 2; break;
            case 'G':  code = 3; break;
            case 'T':  code = 4; break;
            default:   code = -1;
        }
       
        int occVal = occ(curVal,valIdx,code);
        valIdx = occVal+cMap[code];
        //if(i == FILELEN-50) return;
        if(valIdx < bufferStart || valIdx >= bufferStart+BLOCKSIZE)
            seekBuffer(valIdx/BUCKETINTERVAL,valIdx);
        curVal = buffer[valIdx-bufferStart];
    }
    if (remChar > 0){
        fseek(outfile,0,SEEK_SET); 
        char *remDecode = malloc(sizeof(char)*remChar);
        strncpy(remDecode,decoded+(BUFFERSIZE-remChar),remChar);
        fputs(remDecode,outfile);
        free(remDecode);
    }

}

int main (int argc, char* argv[])
{
    indes = open(argv[1],O_RDONLY);
    infile = fopen(argv[1],"r");
    outfile = fopen(argv[2],"w");
    if(infile == NULL){
        printf("Can't open file\n");
        exit(0); 
    }
    initializeGlobal();
    if(FILELEN < SMALLFILE+1){
        bufferStart = 0; 
        fseek(infile,bufferStart,SEEK_SET);
        fread(buffer, FILELEN,1,infile);
        int startIdx = constructTablesS(buffer);
        decodeInputS(startIdx,buffer);
    } else {
        int startIdx = constructTablesL();
        decodeInputL(startIdx);
    }
    fclose(infile);
    close(indes);
    fclose(outfile);
    cleanupGlobal();
    return 0;
}