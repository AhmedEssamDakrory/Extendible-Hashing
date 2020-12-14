#include "Bucket.h"
#include "File.h"
#include <math.h>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
using namespace std;

class ExtendibleHashing{
    int fd, directory_fd;

    //utility functions
    
    inline int getOffset(int idx) {
        return idx*sizeof(Bucket);
    }
    bool deleteOffset(int offset);
    int putIfBucketNotFull(const DataItem& dataItem, int offset);
    void splitBucket(int, Bucket);
    int createNewBucket();
    int doubleDirectory();
    void collisionDetected();
    bool canMerge(const Bucket& b1, const Bucket& b2);
    Bucket merge(const Bucket& b1, const Bucket& b2);
    void shrinkAndCompineAdresses(int BucketAddr, int siblingBucketAddr);
    void halveDiectorySize();
    int hashFn(int);
    int getGlobalDepth();

    void intializeFiles();

    public:

    ExtendibleHashing(int fd, int intdirectory_fd);
    int insert(const DataItem& dataItem);
    int search(const DataItem& dataItem);
    bool deleteItem(const DataItem& dataItem);
    void printDB();
    
};
