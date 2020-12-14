#include<iostream>
#include "ExtendibleHashing.h"
using namespace std;

DataItem newDataItem(int key, int data);

int main(){
    int fd = File::createFile(1, "db/fd.db");
    int dir_fd = File::createFile(1, "db/dir.db");
    
    ExtendibleHashing eh(fd, dir_fd);
    
    DataItem dataItem = newDataItem(7, 1);
    int r = eh.insert(dataItem);
    if(r <= 0) return -1;
    eh.printDB();

    dataItem = newDataItem(9, 2);
    r = eh.insert(dataItem);
    if(r <= 0) return -1;
    eh.printDB();

    dataItem = newDataItem(12, 3);
    r = eh.insert(dataItem);
    if(r <= 0) return -1;
    eh.printDB();

    dataItem = newDataItem(4, 4);
    r = eh.insert(dataItem);
    if(r <= 0) return -1;
    eh.printDB();

    dataItem = newDataItem(10, 5);
    r = eh.insert(dataItem);
    if(r <= 0) return -1;
    eh.printDB();

    dataItem = newDataItem(5, 6);
    r = eh.insert(dataItem);
    if(r <= 0) return -1;
    eh.printDB();

    dataItem = newDataItem(1, 7);
    r = eh.insert(dataItem);
    if(r <= 0) return -1;
    eh.printDB();

    eh.deleteItem(dataItem);
    eh.printDB();

    return 0;
}

DataItem newDataItem(int key, int data) {
    DataItem dataItem;
    dataItem.data = data;
    dataItem.key = key;
    dataItem.valid = 1;
    return dataItem;
}