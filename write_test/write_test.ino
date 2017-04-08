/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO 
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino model, check
  the Technical Specs of your board  at https://www.arduino.cc/en/Main/Products
  
  This example code is in the public domain.

  modified 8 May 2014
  by Scott Fitzgerald
  
  modified 2 Sep 2016
  by Arturo Guadalupi
  
  modified 8 Sep 2016
  by Colby Newman
*/

#include <LiquidCrystal.h>

//LiquidCrystal lcd(12, 11, 10, 5, 4, 3, 2); 

#define RED (3)        /* Red color pin of RGB LED */
#define GREEN (5)      /* Green color pin of RGB LED */
#define BLUE (6)       /* Blue color pin of RGB LED */


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
//  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);  // opens serial port, sets data rate to 9600 bps
    // set up the LCD's number of columns and rows:
//    lcd.begin(16, 1);
     pinMode(BLUE, OUTPUT); 
}

// the loop function runs over and over again forever
void loop() {
  
  int input,num;

    if(Serial.available()>0)
    {
        char buffer[256];
        Serial.readBytes(buffer, 256);

        if(buffer[0] == '1')
        {
              digitalWrite(BLUE, HIGH);
//            lcd.print(buffer[0]); 
//            digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
//            delay(1000);                       // wait for a second
//            digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
//            delay(1000);                       // wait for a second
        }
    }
}
