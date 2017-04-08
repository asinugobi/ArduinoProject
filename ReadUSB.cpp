#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <string> 
using namespace std;


/*
This code configures the file descriptor for use as a serial port.
*/
void configure(int fd) {
  struct  termios pts;
  tcgetattr(fd, &pts);
  cfsetospeed(&pts, 9600);   
  cfsetispeed(&pts, 9600);   
  tcsetattr(fd, TCSANOW, &pts);
}

string temperature;
string units;


int main(int argc, char *argv[]) {

  if (argc < 2) {
    cout << "Please specify the name of the serial port (USB) device file!" << endl;
    exit(0);
  }

  // get the name from the command line
  char* file_name = argv[1];
  
  // try to open the file for reading and writing
  int fd = open(argv[1], O_RDWR | O_NOCTTY | O_NDELAY);
  
  if (fd < 0) {
    perror("Could not open file");
    exit(1);
  }
  else {
    cout << "Successfully opened " << argv[1] << " for reading/writing" << endl;
  }

  configure(fd);

  /*
    Write the rest of the program below, using the read and write system calls.
  */

  int size = 20;
  char buf[size];
  string line; 
  string temp; 
  int n = 0; 
  int pos = 0; 

  while((n = read(fd,buf,size))){
      if(n < 1){
          continue;
      }
      
      line.append(buf);
      
      pos = line.find('\n');
      if(pos != string::npos){
          
          temp = line.substr(0, pos);
          
          int left_side = temp.find("The temperature is ");
          int right_side = temp.find(" degrees");
          if(left_side != string::npos && right_side != string::npos){
              temperature = temp.substr(19, (right_side - 19));
              units = temp.substr((right_side + 9), 1);
              cout << "The weather is: '" << temperature << "' in '" << units << "'" << endl;
          }
          line.clear();
          memset(buf, 0, size);
      } else {
          
      }
      
  }
  
}


