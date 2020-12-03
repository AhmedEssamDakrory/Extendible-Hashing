#include<iostream>
#include "ExtendibleHashing.h"
using namespace std;

int main(){
    int fd = File::createFile(1, "fd.db");
    int dir_fd = File::createFile(1, "dir.db");
    ExtendibleHashing eh(fd, dir_fd);
    DataItem dataItem;
    dataItem.data = 213;
    dataItem.key = 3;
    dataItem.valid = 1;
    eh.insert(dataItem);
    eh.printDB();
}