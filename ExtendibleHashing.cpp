#include "ExtendibleHashing.h"



ExtendibleHashing :: ExtendibleHashing(int fd, int directory_fd){
    this->fd = fd;
    this->directory_fd = directory_fd;
}

void ExtendibleHashing :: deleteOffset(int offset){
    DataItem dummy;
    dummy.data = 0;
    dummy.key = -1;
    dummy.valid = 0;
    ssize_t result = pwrite(this->fd, &dummy, sizeof(DataItem), offset);
} 

int ExtendibleHashing :: putIfBucketNotFull(const DataItem& dataItem, int offset){
    ssize_t result;
    int valid;
    for(int i = 0; i < ITEMS_PER_BUCKET; ++i){
        result = pread(fd, &valid, sizeof(int), offset+2*sizeof(int));
        if(!valid){
            result = pwrite(fd, &dataItem, sizeof(DataItem), offset);
            return (result <= 0) ? ERROR : DONE;  
        }
        offset += sizeof(DataItem);
    }
    return FULL;
}

void ExtendibleHashing :: doubleDirectory(){
    // doubling directory....
    int currentSize = File::getFileSize(this->directory_fd);
    File::extendFile(this->directory_fd, currentSize*2);
    // creating new bucket
    int mainFileSize = File::getFileSize(this->fd);
    File::extendFile(this->fd, mainFileSize+sizeof(Bucket));
    int newIdx = mainFileSize/sizeof(Bucket);
    // pointing to the new bucket..
    for(int offset = currentSize; offset < currentSize*2; offset += sizeof(int)){
        ssize_t result = pwrite(this->directory_fd, &newIdx, sizeof(int), offset);
    }
}

void ExtendibleHashing :: splitBucket(int idx, const DataItem& dataItem){
    //get local depth;
    int localDepth;
    int offset = getOffset(idx);
    int globalDepth = log2(File::getFileSize(this->directory_fd)/sizeof(int));
    ssize_t result = pread(this->fd, &localDepth, sizeof(int), offset+2*sizeof(int));
    if(localDepth == globalDepth){
        this->doubleDirectory();
    }
    // split ....
   
}

void ExtendibleHashing :: insert(const DataItem& dataItem){
}