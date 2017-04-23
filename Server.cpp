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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <vector> 
#include <climits> 
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

// Global variables for the temperature
string temperature;
string units;
string filename;
int fd;
bool fahrenheit = false;
bool stand_by = true;
bool arduino_connected = false;
vector<double> temperatures; 

void* get_temp(void*);
void write_to_device(string);


void arduino_connect(){
	cout << "Attempting to open " << filename << " for reading/writing" << endl;
	
	const char* f = filename.c_str();
	
  // try to open the file for reading and writing
  fd = open(f, O_RDWR | O_NOCTTY | O_NDELAY);
  
  if (fd < 0) {
    perror("Could not open file");
    // exit(1);
  }
  else {
    cout << "Successfully opened " << filename << " for reading/writing" << endl;
  }

  configure(fd);
}

bool arduino_check_connection(){
	int size = 0;
  char buf[1];
  int n = 0; 
  
  n = read(fd,buf,size);
	
	if(n < 0){
		return false;
	} else {
		return true;
	}
}

double get_max_temperature(){
	double max = INT_MIN; 
	for(int i = 0; i < temperatures.size(); i++){
		if(temperatures[i] > max){
			max = temperatures[i];
		}
	}

	return max; 
}

double get_min_temperature(){
	double min = INT_MAX; 
	for(int i = 0; i < temperatures.size(); i++){
		if(temperatures[i] < min){
			min = temperatures[i];
		}
	}

	return min; 
}

double get_average_temperature(){
	double total = 0; 
	for(int i = 0; i < temperatures.size(); i++){
		total += temperatures[i];
	}

	return total/temperatures.size(); 
}

int start_server(int PORT_NUMBER)
{

    // structs to represent the server and client
    struct sockaddr_in server_addr,client_addr;    
    
    int sock; // socket descriptor

    // 1. socket: creates a socket descriptor that you later use to make other system calls
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("Socket");
			exit(1);
    }
    int temp;
    if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&temp,sizeof(int)) == -1) {
			perror("Setsockopt");
			exit(1);
    }

    // configure the server
    server_addr.sin_port = htons(PORT_NUMBER); // specify port number
    server_addr.sin_family = AF_INET;         
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    bzero(&(server_addr.sin_zero),8); 
    
    // 2. bind: use the socket and associate it with the port number
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
			perror("Unable to bind");
			exit(1);
    }

    // 3. listen: indicates that we want to listn to the port to which we bound; second arg is number of allowed connections
    if (listen(sock, 5) == -1) {
			perror("Listen");
			exit(1);
    }
        
    // once you get here, the server is set up and about to start listening
    cout << endl << "Server configured to listen on port " << PORT_NUMBER << endl;
    fflush(stdout);
   	
		int continue_serving_clients = 1;

		while(continue_serving_clients > 0){	
			
		// 4. accept: wait here until we get a connection on that port
	    int sin_size = sizeof(struct sockaddr_in);
	    int fds = accept(sock, (struct sockaddr *)&client_addr,(socklen_t *)&sin_size);
	    cout << "Server got a connection from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << endl;
	    
	    // buffer to read data into
	    char request[1024];
	    
	    // 5. recv: read incoming message into buffer
	    int bytes_received = recv(fds,request,1024,0);
	    // null-terminate the string
	    request[bytes_received] = '\0';
	    // cout << "Here comes the message:" << endl;
	    // cout << request << endl;
	
		string reply;
	    
		// Parse GET Response
		string request_string = request;
		int request_start = request_string.find('/');
      	if(request_start != string::npos){
				int request_end = request_string.substr(request_start, request_string.length() - request_start).find(' ');
				
				string request_action = request_string.substr(request_start + 1, request_end);
				
				cout << "Request Action: " << request_action << endl;
				
				if(arduino_check_connection()){
					
					if(stand_by == true && request_action.find("StandBy") != string::npos) {
						reply = "{\n\"response_type\":\"StandBy\"\n}";
					} else {
						if(request_action.find("getTemp") != string::npos){
							// Return the temperature
							reply = "{\n\"response_type\":\"getTemp\",\n\"temperature\": \"" + temperature + "\",\n\"units\":\"" + units + "\"\n}";
							if(fahrenheit)
								write_to_device("f");
							else
								write_to_device("c");
						}
						else if(request_action.find("StandBy") != string::npos || stand_by == true){
							// Toggle standby mode
							if(stand_by){
								// Exit standby mode
								stand_by = false;
								reply = "{\n\"response_type\":\"StandByExit\"\n}";
								write_to_device("c");
                fahrenheit = false;
							} else {
								// Enter standby mode
								stand_by = true;
								reply = "{\n\"response_type\":\"StandBy\"\n}";
								write_to_device("s");
							}
						} 
						else if(request_action.find("changeUnits") != string::npos){
							reply = "{\n\"response_type\":\"changeUnits\"\n}";
              
							fahrenheit = !fahrenheit;
	
							// Change to fahrenheit
							if(fahrenheit)
								write_to_device("f");
							else
								write_to_device("c");
							
						}
						else if(request_action.find("blue") != string::npos){
							write_to_device("b");
							
						}	
						else if(request_action.find("green") != string::npos){
							write_to_device("g");
							
						}
						else if(request_action.find("red") != string::npos){
							write_to_device("r");
							
						}
						else if(request_action.find("avg") != string::npos){
              double raw_temp = get_average_temperature();
              string unit = "C";
              if(fahrenheit){
                raw_temp = ((9.0/5.0) * raw_temp) + 32.0;
                unit = "F";
              }
              
              reply = "{\n\"response_type\":\"temperatureReportAvg\",\n\"temperature_avg\": \"" + to_string(raw_temp) + "\",\n\"units\":\"" + unit + "\"\n}";
							
						}	
						else if(request_action.find("min") != string::npos){
							double raw_temp = get_min_temperature();
              string unit = "C";
              if(fahrenheit){
                raw_temp = ((9.0/5.0) * raw_temp) + 32.0;
                unit = "F";
              }
              
              reply = "{\n\"response_type\":\"temperatureReportMin\",\n\"temperature_min\": \"" + to_string(raw_temp) + "\",\n\"units\":\"" + unit + "\"\n}";
							
						}
						else if(request_action.find("max") != string::npos){
							double raw_temp = get_max_temperature();
              string unit = "C";
              if(fahrenheit){
                raw_temp = ((9.0/5.0) * raw_temp) + 32.0;
                unit = "F";
              }
              
              reply = "{\n\"response_type\":\"temperatureReportMax\",\n\"temperature_max\": \"" + to_string(raw_temp) + "\",\n\"units\":\"" + unit + "\"\n}";
							
						}
						else {
							// Unknown response
							reply = "{\n\"response_type\":\"A\"\n}";
						}
					}
				
				} else {
					reply = "{\n\"response_type\":\"arduinoNotConnected\"\n}";
					arduino_connect();
					stand_by = true;
				}
				
      } 
      else {
      	// Could not parse request
      	reply = "{\n\"response_type\":\"request_error\"\n}";
      }
			
			cout << "Reply: '" << reply << "'" << endl;
			
	    // 6. send: send the message over the socket
	    // note that the second argument is a char*, and the third is the number of chars
	    int s = send(fds, reply.c_str(), reply.length(), 0);
		
			
	    // 7. close: close the socket connection
	    close(fds);
			
		}
		
		

    
    close(sock);
    cout << "Server closed connection" << endl;

    return 0;
} 


bool check_temperatures(){
	if(temperatures.size() >= 3600){
		return true;
	}
	return false; 
}


void* get_temp(void* p){

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
              
              double raw_temp = stod(temperature);
              if(fahrenheit){
                raw_temp = ((5.0/9.0) * raw_temp) - 32.0;
              }
              
              temperatures.push_back(raw_temp);

              if(check_temperatures()){
              	temperatures.erase(temperatures.begin()); 
              }

              units = temp.substr((right_side + 9), 1);
              cout << "The weather is: '" << temperature << "' in '" << units << "'" << endl;
          }
          line.clear();
          memset(buf, 0, size);
      } else {
          
      }
      
		}
		
		return p;
}

void write_to_device(string message){
	char* a;
  if(message == "c")
  	a = "c";
  else if(message == "f")
  	a = "f";
  else
  	a = "s";

  cout << "Writing " << message << " to the device" << endl;
  int w = write(fd, a, 1);
  cout << "Result: " << w << endl;
}



int main(int argc, char *argv[]) {

  if (argc < 3) {
		
    cout << "Usage: [serial port (USB) device file] [port number]" << endl;
    exit(0);
  }
  
  
  
  char* a = argv[1];
  filename = argv[1];
	
	
	arduino_connect();
	
	
	int r = 0;
	pthread_t t1;
  
  
	r = pthread_create(&t1, NULL, &get_temp, a);
	
	if(r != 0){
    // Thread was not successful!
  }
	
	
	// Start the server
	int PORT_NUMBER = atoi(argv[2]);
  start_server(PORT_NUMBER);
  
}


