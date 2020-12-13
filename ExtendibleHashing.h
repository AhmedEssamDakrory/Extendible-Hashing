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
    void deleteOffset(int offset);
    int splitBucket(int, Bucket);
    int createNewBucket();
    int doubleDirectory();
    
    int hashFn(int);
    int getGlobalDepth();

    void intializeFiles();

    public:

    ExtendibleHashing(int fd, int intdirectory_fd);
    int insert(const DataItem& dataItem);
    int printDB();
    
};