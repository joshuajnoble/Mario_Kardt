/*
 * WiFlyHQ Example WebSocketClient.ino
 *
 * This sketch implements a simple websocket client that connects to a 
 * websocket echo server, sends a message, and receives the response.
 * Accepts a line of text via the serial monitor, sends it to the websocket
 * echo server, and receives the echo response.
 * See http://www.websocket.org for more information.
 *
 * This sketch is released to the public domain.
 *
 */

#include <Arduino.h>
#include <WiFlyHQ.h>
#include <AltSoftSerial.h>

//////////////////////////////////////////////////////////////////
// Wifly
//////////////////////////////////////////////////////////////////

AltSoftSerial wifiSerial(9, 10);
#undef PROGMEM 
#define PROGMEM __attribute__(( section(".progmem.data") )) 
#undef PSTR 
#define PSTR(s) (__extension__({static prog_char __c[] PROGMEM = (s); &__c[0];})) 

int getMessage(char *buf, int size);
void send(const char *data);
bool connect(const char *hostname, const char *path="/", uint16_t port=80);

WiFly wifly;

const char mySSID[] = "frogwirelessext";
const char myPassword[] = "friedolin";

char server[] = "10.118.73.111";
int port = 3000;

//////////////////////////////////////////////////////////////////
// Motor
//////////////////////////////////////////////////////////////////

#define PWMA 14
#define PWMB 15
#define AIN1 1
#define AIN2 0
#define BIN1 4
#define BIN2 3
#define STBY 2
#define MOTOR_A 0
#define MOTOR_B 1
#define FORWARD 1
#define REVERSE 0
#define RIGHT 1
#define LEFT 0

//////////////////////////////////////////////////////////////////
// setup
//////////////////////////////////////////////////////////////////

void setup()
{
    Serial.begin(57600);

    //////////////////////////////////////////////////////////////////
    // set up motor

    pinMode(PWMA,OUTPUT);
    pinMode(AIN1,OUTPUT);
    pinMode(AIN2,OUTPUT);
    pinMode(PWMB,OUTPUT);
    pinMode(BIN1,OUTPUT);
    pinMode(BIN2,OUTPUT);
    pinMode(STBY,OUTPUT);
    
    motor_standby(false);        //Must set STBY pin to HIGH in order to move

    //////////////////////////////////////////////////////////////////
    // set up wifly

    char buf[32];
    wifiSerial.begin(9600);
    if (!wifly.begin(&wifiSerial, &Serial)) {
        Serial.println(F("Failed to start wifly"));
	wifly.terminal();
    }

    /* Join wifi network if not already associated */
    if (!wifly.isAssociated()) {
	Serial.println(F("Joining network"));
	if (wifly.join(mySSID, myPassword, true)) {
	    wifly.save();
	    Serial.println(F("Joined wifi network"));
	} else {
	    Serial.println(F("Failed to join wifi network"));
	    wifly.terminal();
	}
    } else {
        Serial.println(F("Already joined network"));
    }

    if (!connect(server, "", 3000)) {
	Serial.print(F("Failed to connect to "));
	Serial.println(server);
	wifly.terminal();
    }

    Serial.println(F("Sending Hello World"));
    send("Hello, World!");
}

char inBuf[9];
char outBuf[128];
uint8_t outBufInd = 0;

uint16_t left = 127, right = 127;

void loop() 
{
    if (getMessage(inBuf, sizeof(inBuf)) > 0) {
      
      Serial.println(inBuf);
      
      char t[3];
      memset(t, 0x20, 3);
      
      boolean leading = true;
      int i = 0;
      while( i < 3 ) {
        char c = (char) inBuf[i];
        if(c != '0'){
          leading = false;
        }
        if(!leading) {
          t[i] = c;
        }
        i++;
      }
      
      left = (int) strtol(&t[0], NULL, 0);
      
      memset(t, 0x20, 3);
      leading = true;
      
      i = 4;
      while( i < 7 ) {
        char c = (char) inBuf[i];
        if(c != '0'){
          leading = false;
        }
        if(!leading) {
          t[i-4] = c;
        }
        i++;
      }
      
      right = (int) strtol(&t[0], NULL, 0);
      
      Serial.println(left);
      Serial.println(right);
    }
    
    //void motor_control(char motor, char direction, unsigned char speed)
    if(left > 127) {
      motor_control( MOTOR_A, FORWARD, left - 127);
    } else {
      motor_control( MOTOR_A, REVERSE, 127 - left);
    }
    
    if(right > 127) {
      motor_control( MOTOR_B, FORWARD, right - 127);
    } else {
      motor_control( MOTOR_B, REVERSE, 127 - right);
    }
    
}

//////////////////////////////////////////////////////////////////
// Motor
//////////////////////////////////////////////////////////////////

//Turns off the outputs of the Motor Driver when true
void motor_standby(char standby)
{
  if (standby == true) digitalWrite(STBY,LOW);
  else digitalWrite(STBY,HIGH);
}

void motor_control(char motor, char direction, unsigned char speed)
{ 
  if (motor == MOTOR_A)
  {
    if (direction == FORWARD)
    {
      digitalWrite(AIN1,HIGH);
      digitalWrite(AIN2,LOW);
    } 
    else 
    {
      digitalWrite(AIN1,LOW);
      digitalWrite(AIN2,HIGH);
    }
    analogWrite(PWMA,speed);
  }
  else
  {
    if (direction == FORWARD)  //Notice how the direction is reversed for motor_B
    {                          //This is because they are placed on opposite sides so
      digitalWrite(BIN1,LOW);  //to go FORWARD, motor_A spins CW and motor_B spins CCW
      digitalWrite(BIN2,HIGH);
    }
    else
    {
      digitalWrite(BIN1,HIGH);
      digitalWrite(BIN2,LOW);
    }
    analogWrite(PWMB,speed);
  }
}


//////////////////////////////////////////////////////////////////
// WiFly
//////////////////////////////////////////////////////////////////

int getMessage(char *buf, int size)
{
    int len = 0; 

    if (wifly.available() > 0) {
	if (wifly.read() == 0) {
	    /* read up to the end of the message (255) */
	    len = wifly.getsTerm(buf, size, 255);
	}
    }
    return len;
}

void send(const char *data) 
{
    wifly.print((uint8_t)0);
    wifly.print(data);
    wifly.print((uint8_t)255);
}

bool connect(const char *hostname, const char *path, uint16_t port)
{
    if (!wifly.open(hostname, 3000)) {
        Serial.println(F("connect: failed to open TCP connection"));
	return false;
    }
    
    wifly.println(F("GET / HTTP/1.1"));
    wifly.println(F("Upgrade: WebSocket"));
    wifly.println(F("Connection: Upgrade"));
    wifly.print(F("Host: "));
    wifly.print(hostname);
    wifly.println(F("\r\n"));
    wifly.println(F("Origin: 192.168.1.106"));
    wifly.println(F("\r\n"));
    
    // Wait for the handshake response
    if (wifly.match(F("HTTP/1.1 101"), 4000)) {
	wifly.flushRx(200);
	return true;
    }

    Serial.println(F("connect: handshake failed"));
    wifly.flushRx(200);
    wifly.close();

    return false;
}
