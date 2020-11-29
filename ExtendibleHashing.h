#include "Bucket.h"
#include"File.h"
#include<math.h>
#include<vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
using namespace std;

class ExtendibleHashing{
    int fd, directory_fd;
    int globalDepth;

    //utility functions
    
    inline int getOffset(int idx) {
        return idx*sizeof(Bucket);
    }
    void deleteOffset(int offset);
    int putIfBucketNotFull(const DataItem& dataItem, int offset);
    void splitBucket(int idx, const DataItem& dataItem);
    void doubleDirectory();
    void collisionDetected();
    
    public:

    ExtendibleHashing(int fd, int intdirectory_fd);
    void insert(const DataItem& dataItem);
    
};