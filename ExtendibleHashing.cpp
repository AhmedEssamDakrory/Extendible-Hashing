#include "ExtendibleHashing.h"
#include <iostream>
#include<set>
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
    
    File::changeSize(this->directory_fd, sizeof(int)*2);

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


bool ExtendibleHashing :: deleteOffset(int offset){
    DataItem dummy;
    dummy.data = 0;
    dummy.key = -1;
    dummy.valid = 0;
    ssize_t result = pwrite(this->fd, &dummy, sizeof(DataItem), offset);
    if(result < 0){
        perror("Error while writing!");
        return false;
    }
    return true;
} 

int ExtendibleHashing::createNewBucket() {
    int mainFileSize = File::getFileSize(this->fd);

    int newBucketAddr;
    if(mainFileSize<sizeof(Bucket)) {
        File::changeSize(this->fd, sizeof(Bucket));
        newBucketAddr = 0;
    }
    else {
        File::changeSize(this->fd, mainFileSize+sizeof(Bucket));
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
    File::changeSize(this->directory_fd, currentSize*2);
    
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

void ExtendibleHashing::splitBucket(int dir, Bucket b){
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
        return;
    }

    // expand the db and create a new bucket and return its address    
    int newBucketAddr = createNewBucket();

    // edit the directory to make it point to the new bucket
    r = pwrite(this->directory_fd, &newBucketAddr, sizeof(int), newBucketDir*sizeof(int));
    if(r <= 0) {
        perror("Error with pwrite");
        return;
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
        return;
    }
    r = pwrite(this->fd, &new_b2, sizeof(Bucket), newBucketAddr);
    if(r <= 0) {
        perror("Error with pwrite");
        return;
    }
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

int ExtendibleHashing::search(const DataItem& dataItem){
	int offset = -1;
	int globalDepth = getGlobalDepth();

	int h = hashFn(dataItem.key);
	int mask = pow(2, globalDepth) - 1;
	int dir = h & mask;
	int bucketAddr;
	ssize_t r = pread(this->directory_fd, &bucketAddr, sizeof(int), dir*sizeof(int));
	Bucket b;
	r = pread(this->fd, &b, sizeof(Bucket), bucketAddr);
	for(int i = 0; i < ITEMS_PER_BUCKET; i++){
	     DataItem* d = &b.data[i];
	     if(d->valid == 1 && d->data == dataItem.data) {
	         offset = bucketAddr+i*sizeof(DataItem);
	         break;
	     }
	 }

	return offset;
}

bool ExtendibleHashing :: canMerge(const Bucket& b1, const Bucket& b2){
    int cnt1 = 0, cnt2 = 0;
    bool ok = true;
    for(int i = 0; i < ITEMS_PER_BUCKET; ++i){
        if(b1.data[i].valid) ++cnt1;
        if(b2.data[i].valid) ++cnt2;
    }
    return (cnt1+cnt2 <= ITEMS_PER_BUCKET) ? true : false;
}

Bucket ExtendibleHashing :: merge(const Bucket& b1, const Bucket& b2){
    Bucket b;
    int idx = 0;
    for(int i = 0; i < ITEMS_PER_BUCKET; ++i){
        if(b1.data[i].valid) b.data[idx++] = b1.data[i];
        if(b2.data[i].valid) b.data[idx++] = b2.data[i];
    }
    b.localDepth = b1.localDepth-1;
    return b;
}

void ExtendibleHashing :: halveDiectorySize(){
    set<int> s1, s2;
    int directorySize = File::getFileSize(this->directory_fd);
    for(int offset = 0 ; offset < directorySize; offset+=sizeof(int)){
        int address;
        ssize_t result = pread(this->fd, &address, sizeof(int), offset);
        if(s2.find(address) != s2.end()) continue;
        if(s1.find(address) != s1.end()){
            s1.erase(address);
            s2.insert(address);
        } else {
            s1.insert(address);
        }
    }
    if((int) s1.size() == 0) return;
    File::changeSize(this->directory_fd, directorySize/2);
}

void ExtendibleHashing :: shrinkAndCompineAdresses(int bucketAddr, int siblingBucketAddr){
    int fileSize = File::getFileSize(this->fd);
    int directorySize = File::getFileSize(this->directory_fd);
    Bucket b;
    int lastAddress = fileSize - sizeof(Bucket);
    ssize_t result = pread(this->fd, &b, sizeof(Bucket), lastAddress);
    result = pwrite(this->fd, &b, sizeof(Bucket), siblingBucketAddr);
    File::changeSize(this->fd, fileSize-sizeof(Bucket));
    // change Addresses
    for(int i = 0 ; i*sizeof(int) < directorySize; ++i){
        int address;
        result = pread(this->directory_fd, &address, sizeof(int), i*sizeof(int));
        if(address == lastAddress){
            result = pwrite(this->directory_fd, &siblingBucketAddr, sizeof(int), i*sizeof(int));
        }
        if(address == siblingBucketAddr){
            result = pwrite(this->directory_fd, &bucketAddr, sizeof(int), i*sizeof(int));
        }
    }
}



bool ExtendibleHashing::deleteItem(const DataItem& dataItem){
    int globalDepth = this->getGlobalDepth();
    int offset = this->search(dataItem);
    if(offset < 0) return false; // Key not found
    // Delete offset.
    bool f = this->deleteOffset(offset);
    if(!f) return false; // Error while writing!
    int h = hashFn(dataItem.key);
    int mask = (1 << globalDepth) - 1;
    int dir = h & mask;
    int bucketAddr;
    ssize_t result = pread(this->directory_fd, &bucketAddr, sizeof(int), dir*sizeof(int));
    Bucket b, siblingBucket;
    result = pread(this->fd, &b, sizeof(Bucket), bucketAddr);
    do{
        mask = (1 << b.localDepth)-1;
        int label = mask & dir;
        int siblingLabel =  (1 << (b.localDepth-1)) ^ label;
        int siblingBucketAddr;
        result = pread(this->directory_fd, &siblingBucketAddr, sizeof(int), siblingLabel*sizeof(int));
        result = pread(this->fd, &siblingBucket, sizeof(Bucket), siblingBucketAddr);
        if((siblingBucket.localDepth != b.localDepth) || !this->canMerge(b, siblingBucket)) break;
        Bucket newBucket = this->merge(b, siblingBucket);
        if(bucketAddr > siblingBucketAddr ) swap(bucketAddr, siblingBucketAddr), dir = siblingBucketAddr;
        result = pwrite(this->fd, &newBucket, sizeof(Bucket), bucketAddr);
        // shrink ......
        this->shrinkAndCompineAdresses(bucketAddr, siblingBucketAddr);
        b = newBucket;
    } while(1);
    // If no local depth equals the global depth subtract 1 from the global depth and halve the size of the directory. 
    this->halveDiectorySize();
    return true;
}

void ExtendibleHashing::printDB() {
    int globalDepth = getGlobalDepth();
    cout << "Printing the Database.....\nGlobal Depth = " << globalDepth << endl;

    int bucketAddr = 0;
    Bucket b;
    for(int i=0; i<pow(2,globalDepth); i++){
        ssize_t r = pread(this->directory_fd, &bucketAddr, sizeof(int), i*sizeof(int));
        if(r <= 0){
            perror("Error with pread");
        }
        r = pread(fd, &b, sizeof(Bucket), bucketAddr);
        if(r <= 0){
            perror("Error with pread");
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
}
