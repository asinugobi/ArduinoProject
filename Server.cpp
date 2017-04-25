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
int exit_flag = 0;

// Function pointer to start temperature reading thread
void* get_temp(void*);
void write_to_device(string);

// Helper function to check if a string is a number
bool is_number(string s){
	int dot_count = 0;
	for(int i = 0; i < s.length(); i++){
		if(s.c_str()[i] == '.'){
			dot_count++;
		}
		if(s.c_str()[i] != '0' && s.c_str()[i] != '1' && s.c_str()[i] != '2' && s.c_str()[i] != '3' && s.c_str()[i] != '4' && s.c_str()[i] != '5' && s.c_str()[i] != '6' && s.c_str()[i] != '7' && s.c_str()[i] != '8' && s.c_str()[i] != '9' && s.c_str()[i] != '.'){
			return false;
		}
	}
	if(dot_count == 1){
		return true;
	}
	return false;
}

// Helper function to strip non numeric characters from a number string
string extract_number(string s){
	string filtered;
	int dot_count = 0;
	for(int i = 0; i < s.length(); i++){
		if(s.c_str()[i] == '.'){
			dot_count++;
		}
		if(s.c_str()[i] != '0' && s.c_str()[i] != '1' && s.c_str()[i] != '2' && s.c_str()[i] != '3' && s.c_str()[i] != '4' && s.c_str()[i] != '5' && s.c_str()[i] != '6' && s.c_str()[i] != '7' && s.c_str()[i] != '8' && s.c_str()[i] != '9' && s.c_str()[i] != '.'){
			
		} else if(dot_count < 2) {
			filtered += s.c_str()[i];
		}
	}
	return filtered;
}

// Function to initialize a connection to the Arduino
void arduino_connect(){
	const char* f = filename.c_str();
	
  // try to open the file for reading and writing
  fd = open(f, O_RDWR | O_NOCTTY | O_NDELAY);
  
  if (fd < 0) {
    perror("Could not open file");
		return;
  }

  configure(fd);
}

// Function to check if the Arduino is still connected
bool arduino_check_connection(){
  // Read a buffer of size 0
	int size = 0;
  char buf[1];
  int n = 0; 
  
  char a = 'z';
  n = write(fd, &a, 0);
  if(n < 0){
		// Error - Not Connected
		return false;
	} else {
		// Read 0 bytes - Connected
		return true;
	}
}

// Calculate the max temperature
double get_max_temperature(){
	double max = INT_MIN; 
	for(int i = 0; i < temperatures.size(); i++){
		if(temperatures[i] > max){
			max = temperatures[i];
		}
	}

	return max; 
}

// Calculate the min temperature
double get_min_temperature(){
	double min = INT_MAX; 
	for(int i = 0; i < temperatures.size(); i++){
		if(temperatures[i] < min){
			min = temperatures[i];
		}
	}

	return min; 
}

// Calculate the average temperature
double get_average_temperature(){
	double total = 0; 
	for(int i = 0; i < temperatures.size(); i++){
		total += temperatures[i];
	}

	return total/temperatures.size(); 
}

// Starting the server
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
    fflush(stdout);
   	
		
		// Continue accepting conenctions until quit
		while(exit_flag != 1){	
			
			// 4. accept: wait here until we get a connection on that port
	    int sin_size = sizeof(struct sockaddr_in);
	    int fds = accept(sock, (struct sockaddr *)&client_addr,(socklen_t *)&sin_size);
	    
			// buffer to read data into
	    char request[1024];
	    
	    // 5. recv: read incoming message into buffer
	    int bytes_received = recv(fds,request,1024,0);
	    // null-terminate the string
	    request[bytes_received] = '\0';
	
			string reply;
	    
			// Parse GET Response
			string request_string = request;
			int request_start = request_string.find('/');
      if(request_start != string::npos){
				int request_end = request_string.substr(request_start, request_string.length() - request_start).find(' ');
				
				string request_action = request_string.substr(request_start + 1, request_end);
				
				// Check if the arduino is connected
				if(arduino_check_connection()){
					if(stand_by == true && request_action.find("Standby") == string::npos) {
						// Non-standby request while in standby mode
						reply = "{\n\"response_type\":\"StandBy\"\n}";
					} else {
						if(request_action.find("getTemp") != string::npos){
							// Return the temperature
							reply = "{\n\"response_type\":\"getTemp\",\n\"temperature\": \"" + temperature + "\",\n\"units\":\"" + units + "\"\n}";
						}
						else if(request_action.find("Standby") != string::npos || stand_by == true){
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
	
							// Change the units
							if(fahrenheit)
								write_to_device("f");
							else
								write_to_device("c");
							
						}
						else if(request_action.find("blue") != string::npos){
							// Change to blue
							write_to_device("b");
							reply = "{\n\"response_type\":\"generalMessage\",\"message\":\"Changing to Blue\"\n}";
						}	
						else if(request_action.find("green") != string::npos){
							// Change to green
							write_to_device("g");
							reply = "{\n\"response_type\":\"generalMessage\",\"message\":\"Changing to Green\"\n}";
						}
						else if(request_action.find("red") != string::npos){
							// Change to red
							write_to_device("r");
							reply = "{\n\"response_type\":\"generalMessage\",\"message\":\"Changing to Red\"\n}";
						}
						else if(request_action.find("avg") != string::npos){
							// Get the average temperature
              double raw_temp = get_average_temperature();
              string unit = "C";
              if(fahrenheit){
								// Change units if in farenheit
                raw_temp = ((9.0/5.0) * raw_temp) + 32.0;
                unit = "F";
              }
              
              reply = "{\n\"response_type\":\"temperatureReportAvg\",\n\"temperature_avg\": \"" + to_string(raw_temp) + "\",\n\"units\":\"" + unit + "\"\n}";
							
						}	
						else if(request_action.find("min") != string::npos){
							// Get the min temperature
							double raw_temp = get_min_temperature();
              string unit = "C";
              if(fahrenheit){
								// Change units if in farenheit
                raw_temp = ((9.0/5.0) * raw_temp) + 32.0;
                unit = "F";
              }
              
              reply = "{\n\"response_type\":\"temperatureReportMin\",\n\"temperature_min\": \"" + to_string(raw_temp) + "\",\n\"units\":\"" + unit + "\"\n}";
							
						}
						else if(request_action.find("max") != string::npos){
							// Get the max temperature
							double raw_temp = get_max_temperature();
              string unit = "C";
              if(fahrenheit){
								// Change units if in farenheit
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
					// Arduino not connected
					reply = "{\n\"response_type\":\"arduinoNotConnected\"\n}";
					arduino_connect();
					stand_by = true;
				}
				
      } 
      else {
      	// Could not parse request
      	reply = "{\n\"response_type\":\"request_error\"\n}";
      }
			
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

// Function to check if there are 1 hours worth of readings
bool check_temperatures(){
	if(temperatures.size() >= 3600){
		return true;
	}
	return false; 
}

// Temperature reading loop
void* get_temp(void* p){

  int size = 20;
  char buf[size];
  string line; 
  string temp; 
  int n = 0; 
  int pos = 0; 

	// Read data from the Arduino
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
					
					// Search the line for the key phrases
          if(left_side != string::npos && right_side != string::npos){
            	
							// Extract the temperature and unit strings
            	string temperature_raw = temp.substr(19, (right_side - 19));
							string units_temp = temp.substr((right_side + 9), 1);
              
							// Correct data
							temperature_raw = extract_number(temperature_raw);
							if(temp.c_str()[right_side + 10] == 'C' || temp.c_str()[right_side + 10] == 'F'){
								units_temp = temp.c_str()[right_side + 10];
							}
              
              // Check if valid temperature reading
              if((units_temp == "C" || units_temp == "F") && is_number(temperature_raw)){
                temperature = temperature_raw;
                units = units_temp;
								
                char* null_term = 0;
                double raw_temp = strtod(temperature.c_str(), &null_term);
                
								// Convert and add reading to collection
                if(units == "F"){
                  raw_temp = (5.0/9.0) * (raw_temp - 32.0);
									temperatures.push_back(raw_temp);
								} else if(units == "C"){
									temperatures.push_back(raw_temp);
                }
                
  							// Delete temperatures if needed
                if(check_temperatures()){
                	temperatures.erase(temperatures.begin()); 
                }
                
              }
              
          }
          line.clear();
          memset(buf, 0, size);
      } else {
          
      }
      
		}
		
		return p;
}

// Function to write a key message to the Arduino
void write_to_device(string message){
  const char* a = message.c_str();
  int w = write(fd, a, 1);
}

// Stop Thread
void* stop(void* arg){
  char input = 'z';
  while(exit_flag != 1){
    cout << "Type q to exit: ";
    
    scanf("%s", &input);
    
    if(input	== 'q'){
      write_to_device("s");
      exit_flag = 1;
    }
    
    cout << endl;
  }
	return arg;
}

// Main Start
int main(int argc, char *argv[]) {

  if (argc < 3) {
		// Incorrect parameter usage
    cout << "Usage: [serial port (USB) device file] [port number]" << endl;
    exit(0);
  }
  
  // Extract filename
  char* a = argv[1];
  filename = argv[1];
	
	// Connect to the Arduino
	arduino_connect();
	
	// Start the temperature reading thread
	int r = 0;
	pthread_t t1;
	r = pthread_create(&t1, NULL, &get_temp, a);
	if(r != 0){
    // Thread was not successful!
		exit(1);
  }
  
	// Start the stop thread
  int s = 0;
	pthread_t t2;
	s = pthread_create(&t2, NULL, &stop, a);
	if(s != 0){
    // Thread was not successful!
		exit(1);
  }
	
	// Start the server
	int PORT_NUMBER = atoi(argv[2]);
  start_server(PORT_NUMBER);
  
}
