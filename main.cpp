#include<iostream>
#include<fstream>
#include "ExtendibleHashing.h"
using namespace std;

DataItem newDataItem(int key, int data);

int main(){
    int fd = File::createFile(1, "fd.db");
    int dir_fd = File::createFile(1, "dir.db");
    
    ExtendibleHashing eh(fd, dir_fd);

    printf("Enter the test case file name....\n");
	string fileName; cin>>fileName;
	ifstream cin(fileName);
	int n; cin>>n;
    DataItem dataItem;
	for(int i = 0 ; i < n; ++i){
		string op; cin>>op;
		int key, data;
		if(op == "insert"){
			cin>>key>>data;	
            dataItem = newDataItem(key, data);
			eh.insert(dataItem);
            eh.printDB();		
		} 
		else if(op == "search") {
			cin>>key;
			eh.search(key);
		}
		else{
			cin>>key;
			eh.deleteItem(key);
            eh.printDB();			
		} 
	}
    
   close(fd);
   close(dir_fd);
}

DataItem newDataItem(int key, int data) {
    DataItem dataItem;
    dataItem.data = data;
    dataItem.key = key;
    dataItem.valid = 1;
    return dataItem;
}