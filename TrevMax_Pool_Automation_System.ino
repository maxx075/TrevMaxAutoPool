
/* The TrevMax Pool Automation System is an Arduino/Raspberry Pi based 
 * pool automation system controlled via a website.
 * The website (PHP) and webserver (Apache) are housed on the Raspberry Pi 
 * and communicate with the Arduino through the use of a Java program 
 * which is also housed on the Pi.
 * The code below communicates to the Raspberry Pi through the use of the Java 
 * program via reading writing to the Serial Port.
 * This code controls the pump cycles (8 hour, 12 hour, Continuous On, and Off),
 * pH monitoring and adjustment (via servos),
 * temperature reading and heating of the water,
 * and a weekly user specified chlorine dispenser alarm.
 * The program initially sets the internal clock (RTC) via the system
 * time of the attached computer(Raspberry Pi). The program will continually output
 * the temperature and pH readings (every x seconds) to the serial port
 * which will transfer them to the java program and then to the website. 
 * The serial port will recieve the values (chars and int values from the parsed commands) 
 * from the java program (from the website) that will then trigger an output (pump, heat, etc).
 *  
 * templo = low temperature variable for heater
 * temphi = high temperature variable for pump
 * pos = initiial position for PHup Servo
 * pos2 = initiial position for PHdown Servo
 * RX = Pin 3
 * TX = Pin 5
 * Heater = Pin 6
 * Pump = Pin 4
 * PHup Servo = Pin 7
 * PHdown Servo = Pin 8
 * Chlorine Servo = Pin 9
 */

#include <SoftwareSerial.h>
#include <Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Timer.h"
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <TimeAlarms.h>
#include <DS1307RTC.h>

int tempSensorPin = 2; //declares variable tempSensorPin to pin #2
int pos = 0;    // variable to store the servo position of servo PHup
int pos2 = 0;   // variable to store the servo position of servo PHdown
int pos3 = 0;   // variable to store the servo position of servo chlor
int temphi; //low value of temperature
int templo; //high value of temperature
int H;
int pumpEvent;
String inputstring = "";
String sensorstring = "";
String inputString = "";
boolean input_string_complete = false;
boolean sensor_string_complete = false;
boolean stringComplete = false;
float pH;
const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

#define HEATER_PIN 6 //declares heater relay to pin #6
#define rx 3 //define RX to pin #3                 
#define tx 5 //define TX to pin #5   
#define tensecheat (1000UL * 6) //five second timer for temperature sensor
#define tensecph (1000UL * 5) //ten second timer for temperature sensor
unsigned long rolltimeheat = millis() + tensecheat;
unsigned long rolltimeph = millis() + tensecph;
SoftwareSerial myserial(rx, tx); 
Servo PHup; //Servo that will run when pH is too low, attached to pin #7 
Servo PHdown; //Servo that will run when pH is too low, attached to pin #8
Servo chlor;  //Servo that will trigger when the weekly alarm goes off, attached to pin #9
OneWire ds(tempSensorPin);
DallasTemperature sensors(&ds); //passes oneWire reference to Dallas Temperature
Timer t;
AlarmId id;
tmElements_t tm;


void setup()
{   
  Serial.begin(9600); 
  myserial.begin(9600); 
  inputstring.reserve(10);
  sensorstring.reserve(30);
  PHup.attach(7); //attaches servo PHup to pin #7
  PHdown.attach(8); //attaches servo PHdown to pin #8
  chlor.attach(9);  //attaches servo chlor to pin #9
  sensors.begin();
  pinMode(HEATER_PIN, OUTPUT);//sets the HEATER_PIN as an output
  bool parse=false;
  bool config=false;
  
  // get the date and time the compiler was run
  if (getDate(__DATE__) && getTime(__TIME__)) {
    parse = true;
    // and configure the RTC with this info
    if (RTC.write(tm)) {
      config = true;
    }
  }
  Serial.begin(9600);
  while (!Serial) ; // wait for Arduino Serial Monitor
  delay(200);
  if (parse && config) {
    Serial.print("DS1307 configured Time=");
    Serial.print(__TIME__);
    Serial.print(", Date=");
    Serial.println(__DATE__);
  }  
  else if (parse) {
    Serial.println("DS1307 Communication Error :-{");
    Serial.println("Please check your circuitry");
  }  
  else {
    Serial.print("Could not parse info from the compiler, Time=\"");
    Serial.print(__TIME__);
    Serial.print("\", Date=\"");
    Serial.print(__DATE__);
    Serial.println("\"");
  }

  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if (timeStatus() != timeSet)
    Serial.println("Unable to sync with the RTC");
  else
    Serial.println("RTC has set the system time");
}//setup



void loop()
{  
  //Reads Inputs and Outputs from/to text file
  if (stringComplete)
     {
          String command = inputString.substring(0,2);
          Serial.println("the command is: " + command);
          String output = inputString.substring(3);
          if (command == "hi")
          {
            digitalWrite(HEATER_PIN, LOW); //turn off the heater
            temphi = output.toInt();
            Serial.println("High temp set: " + temphi);
          }
          else if(command == "lo")
          {
            digitalWrite(HEATER_PIN, LOW); //turn off the heater
            templo = output.toInt();
            Serial.println("Low temp set: " + templo);
          }

          else if(command == "hr")
          {
            H = output.toInt();
            Serial.println("Alarm hour set: " + H);  
          }

          else if(command == "pm")
          {
            if (output == "x")
            {
              t.stop(pumpEvent);
              do8();
              Serial.println("Eight hour pump cycle on");
            }

            else if(output == "y")
            {
              t.stop(pumpEvent);
              do12();
              Serial.println("Twelve hour pump cycle on");
            }

            else if(output == "z")
            {
              t.stop(pumpEvent);
              doOn();
              Serial.println("Pump on");
            }

            else if(output == "v")
            {
              t.stop(pumpEvent);
              doOff();
              Serial.println("Pump off");
            }
          }

          else if(command == "al")
          {
            if(output == "a")
            {
               Alarm.alarmRepeat(dowSaturday,H,00,00,SatAlarm);
               Serial.println("Alarm Set: Saturday");
            }

            else if(output == "b")
            {
              Alarm.alarmRepeat(dowSunday,H,00,00,SunAlarm);
              Serial.println("Alarm Set: Sunday");
            }

            else if(output == "c")
            {
              Alarm.alarmRepeat(dowMonday,H,00,00,MonAlarm);
              Serial.println("Alarm Set: Monday");
            }
            else if(output == "d")
            {
              Alarm.alarmRepeat(dowTuesday,H,00,00,TueAlarm);
              Serial.println("Alarm Set: Tuesday");
            }

            else if(output == "e")
            {
              Alarm.alarmRepeat(dowWednesday,H,00,00,WedAlarm);
              Serial.println("Alarm Set: Wednesday");
            }

            else if(output == "f")
            {
              Alarm.alarmRepeat(dowThursday,H,00,00,ThuAlarm);
              Serial.println("Alarm Set: Thursday");
            }

            else if(output == "g")
            {
              Alarm.alarmRepeat(dowFriday,H,00,00,FriAlarm);
              Serial.println("Alarm Set: Friday");
            }
            
           }//alarms else if
            
          Serial.println("our new values are: ");
          Serial.print("temphi: ");
          Serial.println(temphi);
          Serial.print("templo: ");
          Serial.println(templo);
          Serial.print("H: ");
          Serial.println(H);     
          inputString = "";
          stringComplete = false;
     }//input & output reading/writing
      t.update();//updates timer for pump cycles
  //Temperature Section//
  if((long)(millis()-rolltimeheat)>=0)
  {
    sensors.requestTemperatures(); // Send the command to get temperatures
    float tempCelsius = sensors.getTempCByIndex(0);
    float temp = ((tempCelsius * 9)/5) + 32;//in degrees F
    String text = "tr:";
    String tempval = text + temp;
    Serial.println(tempval);
    rolltimeheat +=tensecheat;   
    if(temp<templo)
    {
      digitalWrite(HEATER_PIN, HIGH); //turn on the heater
    }
    else if(temp>temphi)
    {
      digitalWrite(HEATER_PIN, LOW); //turn off the heater
    }
  }
  
  //PH Section//
  //Zero PH Servos//
  for (pos = 0; pos <= 0; pos += 1)
  { 
    PHup.write(pos); //tell servo PHup to go to position in variable 'pos' = zeroing
    delay(15); 
  } 

  for (pos2 = 0; pos2 <= 0; pos2 += 1)
  { 
    PHdown.write(pos2); //tell servo PHdown to go to position in variable 'pos2' = zeroing            
    delay(15); 
  }


  //Read pH and determine which Servo to Trigger//
  if (myserial.available() > 0)
  { 
    char inchar = (char)myserial.read();
    sensorstring += inchar;
    if (inchar == '\r')
    {
      sensor_string_complete = true;
    }

  }

  if (sensor_string_complete == true) 
  {
    if (isdigit(sensorstring[0])) 
    {
      pH = sensorstring.toFloat();
      String phtext = "ph:";
      String phvalue = phtext + pH;
      Serial.println(phvalue);
    }

    sensorstring = ""; 
    sensor_string_complete = false;  

    //Trigger Servos According To PH Reading//
    if((long)(millis()-rolltimeph)>=0)
    {
    if(pH < 7.0)
    {
      for (pos = 0; pos <= 0; pos += 1) 
      {
        PHup.write(pos);              
        delay(15); 
      }
      for (pos = 180; pos >= 0; pos -= 1) 
      { 
        PHup.write(pos);              
        delay(15);                       
      }
    }

    if(pH > 8.0)
    {
      for (pos2 = 0; pos2 <= 0; pos2 += 1) 
      { 
        PHdown.write(pos2);            
        delay(15); 
      }
      for (pos2 = 180; pos2 >= 0; pos2 -= 1) 
      {
        PHdown.write(pos2);             
        delay(15);                      
      }
    }
    rolltimeph +=tensecph;
  }
  }//pH Reading
  
  ////Chlorine Servo Zeroing////
  for (pos3 = 0; pos3 <= 0; pos3 += 1)
  { 
    chlor.write(pos3); //tell servo chlor to go to position in variable 'pos3' = zeroing
    delay(15); 
  } //chlorine servo zero
}//loop


void serialEvent()
{
  while(Serial.available())
     {
          char inChar = (char)Serial.read();
          if(inChar == '\n')
               stringComplete = true;
          else
               inputString += inChar;     
     }
}

//Eight Hour Pump Cycle//
void do8()
{
  pinMode(4, OUTPUT);
  pumpEvent = t.oscillate(4, 2000, HIGH);
}

//Twelve Hour Pump Cycle//
void do12()
{
  pinMode(4, OUTPUT);
  pumpEvent = t.oscillate(4, 8000, HIGH);
}

//Continous Pump Cycle//
void doOn()
{
  pinMode(4, OUTPUT);
  digitalWrite(4,HIGH);
}

//Pump OFF//
void doOff()
{
  pinMode(4, OUTPUT);
  digitalWrite(4,LOW);
}

//RTC Set Time//
bool getTime(const char *str)
{
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getDate(const char *str)
{
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}

void SatAlarm() {
  for (pos3 = 0; pos3 <= 0; pos3 += 1) 
  { 
    chlor.write(pos3);            
    delay(15); 
  }
  for (pos3 = 180; pos3 >= 0; pos3 -= 1) 
  {
    chlor.write(pos3);             
    delay(15);                      
  }
}

void SunAlarm() {
  for (pos3 = 0; pos3 <= 0; pos3 += 1) 
  { 
    chlor.write(pos3);            
    delay(15); 
  }
  for (pos3 = 180; pos3 >= 0; pos3 -= 1) 
  {
    chlor.write(pos3);             
    delay(15);                      
  }
}

void MonAlarm() {
  for (pos3 = 0; pos3 <= 0; pos3 += 1) 
  { 
    chlor.write(pos3);            
    delay(15); 
  }
  for (pos3 = 180; pos3 >= 0; pos3 -= 1) 
  {
    chlor.write(pos3);             
    delay(15);                      
  }
}

void TueAlarm() {
  for (pos3 = 0; pos3 <= 0; pos3 += 1) 
  { 
    chlor.write(pos3);            
    delay(15); 
  }
  for (pos3 = 180; pos3 >= 0; pos3 -= 1) 
  {
    chlor.write(pos3);             
    delay(15);                      
  }
}

void WedAlarm() {
  for (pos3 = 0; pos3 <= 0; pos3 += 1) 
  { 
    chlor.write(pos3);            
    delay(15); 
  }
  for (pos3 = 180; pos3 >= 0; pos3 -= 1) 
  {
    chlor.write(pos3);             
    delay(15);                      
  }
}

void ThuAlarm() {
  for (pos3 = 0; pos3 <= 0; pos3 += 1) 
  { 
    chlor.write(pos3);            
    delay(15); 
  }
  for (pos3 = 180; pos3 >= 0; pos3 -= 1) 
  {
    chlor.write(pos3);             
    delay(15);                      
  }
}

void FriAlarm() {
  for (pos3 = 0; pos3 <= 0; pos3 += 1) 
  { 
    chlor.write(pos3);            
    delay(15); 
  }
  for (pos3 = 180; pos3 >= 0; pos3 -= 1) 
  {
    chlor.write(pos3);             
    delay(15);                      
  }
}

void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println();
}

void printDigits(int digits) {
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

