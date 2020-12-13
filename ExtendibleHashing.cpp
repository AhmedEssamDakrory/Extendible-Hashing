#include "ExtendibleHashing.h"
#include <iostream>
using namespace std;

ExtendibleHashing :: ExtendibleHashing(int fd, int directory_fd){
    this->fd = fd;
    this->directory_fd = directory_fd;
    
    int gd = getGlobalDepth();
    cout << gd << endl;
    if(gd==0){
        intializeFiles();
    }
    printDB();
}

void ExtendibleHashing::intializeFiles() {
    
    // create a bucket and save to db file
    int bAddr = createNewBucket();
    cout << bAddr << endl;
    File::extendFile(this->directory_fd, sizeof(int)*2);

    // pointing the new directories to the old buckets..
    for(int i=0; i<2; i++){
        ssize_t r = pwrite(directory_fd, &bAddr, sizeof(int), i*sizeof(int));
        if(r <= 0){
            perror("Error with pwrite");
            return;
        }
    }
}

int ExtendibleHashing::hashFn(int key) {
    key % 13;
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

int ExtendibleHashing::createNewBucket() {
    int mainFileSize = File::getFileSize(this->fd);
    File::extendFile(this->fd, mainFileSize+sizeof(Bucket));
    int newBucketAddr = mainFileSize;
    
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

void ExtendibleHashing :: splitBucket(int dir, Bucket b){
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
    int newBucketDir = dir | (1<<localDepth);

    // get the original bucket address
    int oldBucketAddr;
    ssize_t r = pread(this->directory_fd, &oldBucketAddr, sizeof(int), dir*sizeof(int));

    // expand the db and create a new bucket and return its address    
    int newBucketAddr = createNewBucket();

    // edit the directory to make it point to the new bucket
    r = pwrite(this->directory_fd, &newBucketAddr, sizeof(int), dir*sizeof(int));

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
    r = pwrite(this->fd, &new_b2, sizeof(Bucket), newBucketAddr);

}


int ExtendibleHashing::getGlobalDepth() {
    int fileSize = File::getFileSize(this->directory_fd);
    if(fileSize<sizeof(int)) return 0;
    return log2(fileSize/sizeof(int));
}
 
int ExtendibleHashing :: insert(const DataItem& dataItem){
    int globalDepth = getGlobalDepth();

    int h = hashFn(dataItem.key);
    int mask = pow(2, globalDepth) - 1;
    int dir = h & mask;
    // cout << "Here\n";
    int bucketAddr;
    ssize_t r = pread(this->directory_fd, &bucketAddr, sizeof(int), dir);
    // cout <<"here1\n";
    Bucket b;
    r = pread(this->fd, &b, sizeof(Bucket), bucketAddr);
    // cout << "here2\n";
    for(int i = 0; i < ITEMS_PER_BUCKET; i++){
        DataItem* d = &b.data[i];
        if(d->valid == 0) {
            d->data = dataItem.data;
            d->key = dataItem.key;
            d->valid = 1;
            r = pwrite(fd, &b, sizeof(Bucket), bucketAddr);
            if(r <= 0) {
                perror("Error with pwrite");
                return -1;
            }
            return 0;
        }    
    }
    // cout <<"Here3\n";
    // if reached here, means there's no space in the bucket, so we need to split this bucket.
    
    // if the local depth of the bucket == global depth so we need to double the directory size first.
    if(b.localDepth == globalDepth) {
        doubleDirectory();
        globalDepth = getGlobalDepth();
        mask = pow(2, globalDepth) - 1;
        dir = h & mask;
    }

    // split the bucket and insert the item in the new expanded db
    splitBucket(dir, b);
    insert(dataItem);
}

bool ExtendibleHashing::search(const DataItem& dataItem){
	bool result = false;
	int globalDepth = getGlobalDepth();

	int h = hashFn(dataItem.key);
	int mask = pow(2, globalDepth) - 1;
	int dir = h & mask;
	int bucketAddr;
	ssize_t r = pread(this->directory_fd, &bucketAddr, sizeof(int), dir);
	Bucket b;
	r = pread(this->fd, &b, sizeof(Bucket), bucketAddr);
	for(int i = 0; i < ITEMS_PER_BUCKET; i++){
	     DataItem* d = &b.data[i];
	     if(d->valid == 1 && d->data == dataItem.data) {
	         result = true;
	         break;
	     }
	 }

	return result;
}

bool ExtendibleHashing::deleteItem(const DataItem& dataItem){
	bool result = false;

	bool itemExist = search(dataItem);
	if(itemExist){

	}


	return result;
}

void ExtendibleHashing::printDB() {
    int globalDepth = getGlobalDepth();
    cout << "Printing the Database.....\nGlobal Depth = " << globalDepth << endl;

    int bucketAddr = 0;
    Bucket b;
    for(int i=0; i<pow(2,globalDepth); i++){
        ssize_t r = pread(this->directory_fd, &bucketAddr, sizeof(int), i*sizeof(int));
        r = pread(fd, &b, sizeof(Bucket), bucketAddr);
        cout << "Directory " << i << ": " << bucketAddr << "\nLocal Depth = " << b.localDepth << endl;
        for(int j=0; j<ITEMS_PER_BUCKET; j++){
            if(b.data[j].valid==1) {
                cout << "Slot " << j << ", key = " << b.data[j].key << ", value = " << b.data[j].data << endl;
            }
        }
    }
}
