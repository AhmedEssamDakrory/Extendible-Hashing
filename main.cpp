#include<iostream>
#include "File.h"
using namespace std;
int main(){
    int fd = File::createFile(200, "dd.txt");
    File::extendFile(fd, 400);
    cout<<File::getFileSize(fd)<<endl;
}