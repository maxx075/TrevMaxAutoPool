# TrevMaxAutoPool
TrevMax Pool Automation System

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
