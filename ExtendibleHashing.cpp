#include "ExtendibleHashing.h"
#include <iostream>
using namespace std;

ExtendibleHashing :: ExtendibleHashing(int fd, int directory_fd){
    this->fd = fd;
    this->directory_fd = directory_fd;
    
    int gd = getGlobalDepth();
    cout << "Intiliazing Extendible Hashing\n";
    if(gd==0){
        cout << "Files are Empty, Intializing DB.....\n";
        intializeFiles();
    }
    printDB();
}

void ExtendibleHashing::intializeFiles() {
    
    // create two buckets and save them to db file
    int b1Addr = createNewBucket();
    int b2Addr = createNewBucket();
    
    File::extendFile(this->directory_fd, sizeof(int)*2);

    // pointing the directories to the new buckets..
    ssize_t r = pwrite(directory_fd, &b1Addr, sizeof(int), 0);
    if(r <= 0){
        perror("Error with pwrite");
        return;
    }
    
    r = pwrite(directory_fd, &b2Addr, sizeof(int), sizeof(int));
    if(r <= 0){
        perror("Error with pwrite");
        return;
    }
}

int ExtendibleHashing::hashFn(int key) {
    return key % 101;
}


void ExtendibleHashing :: deleteOffset(int offset){
    DataItem dummy;
    dummy.data = 0;
    dummy.key = -1;
    dummy.valid = 0;
    ssize_t result = pwrite(this->fd, &dummy, sizeof(DataItem), offset);
} 

int ExtendibleHashing::createNewBucket() {
    int mainFileSize = File::getFileSize(this->fd);
    int newBucketAddr;
    if(mainFileSize<sizeof(Bucket)) {
        File::extendFile(this->fd, sizeof(Bucket));
        newBucketAddr = 0;
    }
    else {
        File::extendFile(this->fd, mainFileSize+sizeof(Bucket));
        newBucketAddr = mainFileSize;
    }
    
    Bucket b;
    b.localDepth = 1;
    for(int i=0; i<ITEMS_PER_BUCKET; i++) {
        DataItem d;
        d.valid = 0;
        d.data = 0;
        d.key = 0;

        b.data[i] = d;
    }

    ssize_t r = pwrite(fd, &b, sizeof(Bucket), newBucketAddr);
    if(r <= 0) {
        perror("Error with pwrite");
        return -1;
    }

    return newBucketAddr;
}

int ExtendibleHashing :: doubleDirectory(){
    // doubling directory....
    int currentSize = File::getFileSize(this->directory_fd);
    File::extendFile(this->directory_fd, currentSize*2);
    
    // pointing the new directories to the old buckets..
    for(int offset = currentSize; offset < currentSize*2; offset += sizeof(int)){
        int addr;
        ssize_t r = pread(this->directory_fd, &addr, sizeof(int), offset-currentSize);
        if(r <= 0){
            perror("Error with pread");
            return -1;
        }
        r = pwrite(this->directory_fd, &addr, sizeof(int), offset);
        if(r <= 0){
            perror("Error with pwrite");
            return -1;
        }
    }
    return 0;
}

int ExtendibleHashing::splitBucket(int dir, Bucket b){
    int globalDepth = getGlobalDepth();
    int localDepth = b.localDepth;

    // create two new bucket variable and intialize them
    Bucket new_b1, new_b2;
    for (auto &d: new_b1.data) {
        d.valid = 0;
    }
    for (auto &d: new_b2.data) {
        d.valid = 0;
    }

    new_b1.localDepth = localDepth + 1;
    new_b2.localDepth = localDepth + 1;

    // get the directories for the buckets
    int mask = pow(2, localDepth) - 1;
    int oldBucketDir = dir & mask;
    int newBucketDir = oldBucketDir | (1<<localDepth);

    // get the original bucket address
    int oldBucketAddr;
    ssize_t r = pread(this->directory_fd, &oldBucketAddr, sizeof(int), oldBucketDir*sizeof(int));
    if(r <= 0){
        perror("Error with pread");
        return -1;
    }

    // expand the db and create a new bucket and return its address    
    int newBucketAddr = createNewBucket();

    // edit the directory to make it point to the new bucket
    r = pwrite(this->directory_fd, &newBucketAddr, sizeof(int), newBucketDir*sizeof(int));
    if(r <= 0) {
        perror("Error with pwrite");
        return -1;
    }

    int j=0; int k=0; // counters for new buckets

    for(int i = 0; i < ITEMS_PER_BUCKET; i++){
        DataItem* d = &b.data[i];
        if(d->valid == 1) {
            int h = hashFn(d->key);
            int mask = pow(2, localDepth+1) - 1;
            int ddir = h & mask;
            if(ddir == oldBucketDir) {
                // add to b1
                new_b1.data[j++] = *d;
            }
            else {
                // add to b2
                new_b2.data[k++] = *d;
            }
        }    
    }

    // write buckets to their address
    r = pwrite(this->fd, &new_b1, sizeof(Bucket), oldBucketAddr);
    if(r <= 0) {
        perror("Error with pwrite");
        return -1;
    }
    r = pwrite(this->fd, &new_b2, sizeof(Bucket), newBucketAddr);
    if(r <= 0) {
        perror("Error with pwrite");
        return -1;
    }
    return 0;
}


int ExtendibleHashing::getGlobalDepth() {
    int fileSize = File::getFileSize(this->directory_fd);
    if(fileSize < sizeof(int)*2) return 0;
    return log2(fileSize/sizeof(int));
}
 
int ExtendibleHashing :: insert(const DataItem& dataItem){
    int count = 0;
    int globalDepth = getGlobalDepth();

    int h = hashFn(dataItem.key);
    int mask = pow(2, globalDepth) - 1;
    int dir = h & mask;
    int bucketAddr;
    
    ssize_t r = pread(this->directory_fd, &bucketAddr, sizeof(int), sizeof(int)*dir);
    if(r <= 0){
        perror("Error with pread");
        return -1;
    }

    Bucket b;
    r = pread(this->fd, &b, sizeof(Bucket), bucketAddr);
    if(r <= 0){
        perror("Error with pread");
        return -1;
    }

    for(int i = 0; i < ITEMS_PER_BUCKET; i++){
        DataItem* d = &b.data[i];
        count++;
        if(d->valid == 0) {
            d->data = dataItem.data;
            d->key = dataItem.key;
            d->valid = 1;
            r = pwrite(fd, &b, sizeof(Bucket), bucketAddr);
            if(r <= 0) {
                perror("Error with pwrite");
                return -1;
            }
            cout << "Data Item with key " << dataItem.key << " inserted....\n";
            return count;
        }    
    }
    // if reached here, means there's no space in the bucket, so we need to split this bucket.
    
    // if the local depth of the bucket == global depth so we need to double the directory size first.
    if(b.localDepth == globalDepth) {
        doubleDirectory();
    }

    // split the bucket and insert the item in the new expanded db
    splitBucket(h, b);
    count += insert(dataItem);
    
    return count;
}

int ExtendibleHashing::printDB() {
    int globalDepth = getGlobalDepth();
    cout << "Printing the Database.....\nGlobal Depth = " << globalDepth << endl;

    int bucketAddr = 0;
    Bucket b;
    for(int i=0; i<pow(2,globalDepth); i++){
        ssize_t r = pread(this->directory_fd, &bucketAddr, sizeof(int), i*sizeof(int));
        if(r <= 0){
            perror("Error with pread");
            return -1;
        }
        r = pread(fd, &b, sizeof(Bucket), bucketAddr);
        if(r <= 0){
            perror("Error with pread");
            return -1;
        }
        cout << "Directory " << i << ": Offset = " << bucketAddr << "\nLocal Depth = " << b.localDepth << endl;
        for(int j=0; j<ITEMS_PER_BUCKET; j++){
            if(b.data[j].valid==1) {
                cout << "Slot " << j << ": key = " << b.data[j].key << ", value = " << b.data[j].data << endl;
            }
            else {
                cout << "Slot " << j << ": Empty......\n";
            }
        }
        cout << "--------\n";
    }
    return 0;
}