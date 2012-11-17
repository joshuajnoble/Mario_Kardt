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
// Color Sensor
//////////////////////////////////////////////////////////////////

// ADJD-S311's I2C address, don't change
#define ADJD_S311_ADDRESS 0x74

#define RED 0
#define GREEN 1
#define BLUE 2
#define CLEAR 3

// ADJD-S311's register list
#define CTRL 0x00
#define CONFIG 0x01
#define CAP_RED 0x06
#define CAP_GREEN 0x07
#define CAP_BLUE 0x08
#define CAP_CLEAR 0x09
#define INT_RED_LO 0xA
#define INT_RED_HI 0xB
#define INT_GREEN_LO 0xC
#define INT_GREEN_HI 0xD
#define INT_BLUE_LO 0xE
#define INT_BLUE_HI 0xF
#define INT_CLEAR_LO 0x10
#define INT_CLEAR_HI 0x11
#define DATA_RED_LO 0x40
#define DATA_RED_HI 0x41
#define DATA_GREEN_LO 0x42
#define DATA_GREEN_HI 0x43
#define DATA_BLUE_LO 0x44
#define DATA_BLUE_HI 0x45
#define DATA_CLEAR_LO 0x46
#define DATA_CLEAR_HI 0x47
#define OFFSET_RED 0x48
#define OFFSET_GREEN 0x49
#define OFFSET_BLUE 0x4A
#define OFFSET_CLEAR 0x4B


#include <AltSoftSerial.h> // need this to talk to the wifly shield.
#include <WiFlyHQ.h> // wifly!
#include <Wire.h> // i2c library to talk to the color sensor

// ADJD-S311 Pin definitions:
int sdaPin = 5;  // serial data, hardwired, can't change
int sclPin = 6;  // serial clock, hardwired, can't change
int ledPin = 7;  // LED light source pin, any unused pin will work

// initial values for integration time registers
unsigned char colorCap[4] = {9, 9, 2, 5};  // values must be between 0 and 15
unsigned int colorInt[4] = {2048, 2048, 2048, 2048};  // max value for these is 4095
unsigned int colorData[4];  // This is where we store the RGB and C data values
signed char colorOffset[4];  // Stores RGB and C offset values

//////////////////////////////////////////////////////////////////
// WIFLY
//////////////////////////////////////////////////////////////////

#undef PROGMEM 
#define PROGMEM __attribute__(( section(".progmem.data") )) 
#undef PSTR 
#define PSTR(s) (__extension__({static prog_char __c[] PROGMEM = (s); &__c[0];})) 

bool hasGottenIDFromServer = false;

// Teensy can do regular Serial, so we might just
// want to swap it over, but we'll have to see
AltSoftSerial wifiSerial(10,11);

// Change these to match your WiFi network
const char mySSID[] = "frogwirelessext";
const char myPassword[] = "friedolin";

const int PORT = 3000;
const char IP[] = "10.118.73.72";
long lastCheck;

WiFly wifly;

//////////////////////////////////////////////////////////////////
// COLORS
//////////////////////////////////////////////////////////////////

const int blue[] = { 884, 824, 607, 541 };
const int red[] = { 1000, 655, 427, 534 };
const int yellow[] = { 1023, 860, 524, 617 };
const int orange[] = { 1023, 744, 440, 635 };
const int green[] = { 938, 825, 427, 564 };
const int magenta[] = { 895, 573, 403, 482 };
const int greenLined[] = { 741, 683, 392, 487 };

const int *colors[] = { blue, red, yellow, orange, green, magenta, greenLined };


//////////////////////////////////////////////////////////////////
// GAME LOGIC
//////////////////////////////////////////////////////////////////

bool lastReqResponded = false;

char inBuf[9];
char outBuf[128];
uint8_t outBufInd = 0;

uint16_t left = 127, right = 127;

void setup()
{


  Serial.begin(57600);
  
  // set up the motor driver

  pinMode(PWMA,OUTPUT);
  pinMode(AIN1,OUTPUT);
  pinMode(AIN2,OUTPUT);
  pinMode(PWMB,OUTPUT);
  pinMode(BIN1,OUTPUT);
  pinMode(BIN2,OUTPUT);
  pinMode(STBY,OUTPUT);
  
  pinMode(ledPin, OUTPUT);

  char buf[32];

  /////////////////////////////////////////////////////
  // WIFLY
  /////////////////////////////////////////////////////

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

    if (!connect(IP, "", 3000)) {
      Serial.print(F("Failed to connect to "));
      Serial.println(IP);
      wifly.terminal();
    }

    Serial.println(F("Sending Hello World"));
    send("Hello, World!");

  ////////////////////////////////////////////////////
  // COLOR SENSOR
  ////////////////////////////////////////////////////
  pinMode(ledPin, OUTPUT);  // Set the sensor's LED as output
  digitalWrite(ledPin, HIGH);  // Initially turn LED light source on
  
  Serial.begin(9600);
  Serial.println( "begin wire ");
  Wire.begin();
  Serial.println( "wire begun ");
  
  delay(1);  // Wait for ADJD reset sequence
  initADJD_S311();  // Initialize the ADJD-S311, sets up cap and int registers

  calibrateColor();  // This calibrates R, G, and B int registers
  calibrateClear();  // This calibrates the C int registers
  calibrateCapacitors();  // This calibrates the RGB, and C cap registers
  getRGBC();  // After calibrating, we can get the first RGB and C data readings

  
  motor_standby(false);        //Must set STBY pin to HIGH in order to move
  digitalWrite(ledPin, HIGH);

}


//
void loop()
{
  digitalWrite(ledPin, HIGH);
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

  if(lastCheck - millis() > 100) {
    lastCheck = millis();
    getRGBC();
    checkColors();
  }

}

int checkColors()
{
  
  // find the color we need
  /*for( int i = 0; i < 6; i++) {
    if( abs(colors[i][0] - COLORS[RED] ) < 20 ) {
      if( abs(colors[i][1] - COLORS[GREEN] ) < 20 ) {
        if( abs(colors[i][2] - COLORS[BLUE] ) < 20 ) {
          if( abs(colors[i][3] - COLORS[CLEAR] ) < 20 ) {
            return index;
          }
        }
      }
    }
  }*/
  
  return -1;
}

//////////////////////////////////////////////////////////////////
// WIFLY
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
// ADJDS311
//////////////////////////////////////////////////////////////////

void initADJD_S311()
{ 
  
  /*sensor gain registers, CAP_...
  to select number of capacitors.
  value must be <= 15 */
  writeRegister(colorCap[RED] & 0xF, CAP_RED);
  writeRegister(colorCap[GREEN] & 0xF, CAP_GREEN);
  writeRegister(colorCap[BLUE] & 0xF, CAP_BLUE);
  writeRegister(colorCap[CLEAR] & 0xF, CAP_CLEAR);

  /* Write sensor gain registers INT_...
  to select integration time 
  value must be <= 4096 */
  writeRegister((unsigned char)colorInt[RED], INT_RED_LO);
  writeRegister((unsigned char)((colorInt[RED] & 0x1FFF) >> 8), INT_RED_HI);
  writeRegister((unsigned char)colorInt[BLUE], INT_BLUE_LO);
  writeRegister((unsigned char)((colorInt[BLUE] & 0x1FFF) >> 8), INT_BLUE_HI);
  writeRegister((unsigned char)colorInt[GREEN], INT_GREEN_LO);
  writeRegister((unsigned char)((colorInt[GREEN] & 0x1FFF) >> 8), INT_GREEN_HI);
  writeRegister((unsigned char)colorInt[CLEAR], INT_CLEAR_LO);
  writeRegister((unsigned char)((colorInt[CLEAR] & 0x1FFF) >> 8), INT_CLEAR_HI);
  
  Serial.println(" done initADJD_S311 ");
}

/* calibrateClear() - This function calibrates the clear integration registers
of the ADJD-S311.
*/
int calibrateClear()
{
  int gainFound = 0;
  int upperBox=4096;
  int lowerBox = 0;
  int half;
  
  while (!gainFound)
  {
    half = ((upperBox-lowerBox)/2)+lowerBox;
    //no further halfing possbile
    if (half==lowerBox)
      gainFound=1;
    else 
    {
      writeInt(INT_CLEAR_LO, half);
      performMeasurement();
      int halfValue = readRegisterInt(DATA_CLEAR_LO);

      if (halfValue>1000)
        upperBox=half;
      else if (halfValue<1000)
        lowerBox=half;
      else
        gainFound=1;
    }
  }
  return half;
}

/* calibrateColor() - This function clalibrates the RG and B 
integration registers.
*/
int calibrateColor()
{
  
  Serial.print(" calibrate color ");
  
  int gainFound = 0;
  int upperBox=4096;
  int lowerBox = 0;
  int half;
  
  while (!gainFound)
  {
    half = ((upperBox-lowerBox)/2)+lowerBox;
    //no further halfing possbile
    if (half==lowerBox)
    {
      gainFound=1;
    }
    else {
      writeInt(INT_RED_LO, half);
      writeInt(INT_GREEN_LO, half);
      writeInt(INT_BLUE_LO, half);

      performMeasurement();
      int halfValue = 0;

      halfValue=max(halfValue, readRegisterInt(DATA_RED_LO));
      halfValue=max(halfValue, readRegisterInt(DATA_GREEN_LO));
      halfValue=max(halfValue, readRegisterInt(DATA_BLUE_LO));

      if (halfValue>1000) {
        upperBox=half;
      }
      else if (halfValue<1000) {
        lowerBox=half;
      }
      else {
        gainFound=1;
      }
    }
  }
  return half;
}

/* calibrateCapacitors() - This function calibrates each of the RGB and C
capacitor registers.
*/
void calibrateCapacitors()
{
  
  Serial.print(" calibrate cap ");
  
  int  calibrationRed = 0;
  int  calibrationBlue = 0;
  int  calibrationGreen = 0;
  int calibrated = 0;

  //need to store detect better calibration
  int oldDiff = 5000;

  while (!calibrated)
  {
    // sensor gain setting (Avago app note 5330)
    // CAPs are 4bit (higher value will result in lower output)
    writeRegister(calibrationRed, CAP_RED);
    writeRegister(calibrationGreen, CAP_GREEN);
    writeRegister(calibrationBlue, CAP_BLUE);

    // int colorGain = _calibrateColorGain();
    int colorGain = readRegisterInt(INT_RED_LO);
    writeInt(INT_RED_LO, colorGain);
    writeInt(INT_GREEN_LO, colorGain);
    writeInt(INT_BLUE_LO, colorGain);

    int maxRead = 0;
    int minRead = 4096;
    int red   = 0;
    int green = 0;
    int blue  = 0;
    
    for (int i=0; i<4 ;i ++)
    {
      performMeasurement();
      red   += readRegisterInt(DATA_RED_LO);
      green += readRegisterInt(DATA_GREEN_LO);
      blue  += readRegisterInt(DATA_BLUE_LO);
    }
    red   /= 4;
    green /= 4;
    blue  /= 4;

    maxRead = max(maxRead, red);
    maxRead = max(maxRead, green);
    maxRead = max(maxRead, blue);

    minRead = min(minRead, red);
    minRead = min(minRead, green);
    minRead = min(minRead, blue);

    int diff = maxRead - minRead;

    if (oldDiff != diff)
    {
      if ((maxRead==red) && (calibrationRed<15))
        calibrationRed++;
      else if ((maxRead == green) && (calibrationGreen<15))
        calibrationGreen++;
      else if ((maxRead == blue) && (calibrationBlue<15))
        calibrationBlue++;
    }
    else
      calibrated = 1;
      
    oldDiff=diff;

    int rCal = calibrationRed;
    int gCal = calibrationGreen;
    int bCal = calibrationBlue;
  }
  
  Serial.println(" done calibrate cap ");
  
}

/* writeInt() - This function writes a 12-bit value
to the LO and HI integration registers */
void writeInt(int address, int gain)
{
  if (gain < 4096) 
  {
    byte msb = gain >> 8;
    byte lsb = gain;

    writeRegister(lsb, address);
    writeRegister(msb, address+1);
  }
}

/* performMeasurement() - This must be called before
reading any of the data registers. This commands the
ADJD-S311 to perform a measurement, and store the data
into the data registers.*/
void performMeasurement()
{  
  writeRegister(0x01, 0x00); // start sensing
  while(readRegister(0x00) != 0)
    ; // waiting for a result
}

/* getRGBC() - This function reads all of the ADJD-S311's
data registers and stores them into colorData[]. To get the
most up-to-date data make sure you call performMeasurement() 
before calling this function.*/
void getRGBC()
{
  performMeasurement();
  
  colorData[RED] = readRegisterInt(DATA_RED_LO);
  colorData[GREEN] = readRegisterInt(DATA_GREEN_LO);
  colorData[BLUE] = readRegisterInt(DATA_BLUE_LO);
  colorData[CLEAR] = readRegisterInt(DATA_CLEAR_LO);
}

/* getOffset() - This function performs the offset reading
and stores the offset data into the colorOffset[] array.
You can turn on data trimming by uncommenting out the 
writing 0x01 to 0x01 code.
*/
void getOffset()
{
  digitalWrite(ledPin, LOW);  // turn LED off
  delay(10);  // wait a tic
  writeRegister(0x02, 0x00); // start sensing
  while(readRegister(0x00) != 0)
    ; // waiting for a result
  //writeRegister(0x01, 0x01);  // set trim
  //delay(100);
  for (int i=0; i<4; i++)
    colorOffset[i] = (signed char) readRegister(OFFSET_RED+i);
  digitalWrite(ledPin, HIGH);
}

////////////////////////////////////////////
// I2C functions...
////////////////////////////////////////////

// Write a byte of data to a specific ADJD-S311 address
void writeRegister(unsigned char data, unsigned char address)
{
  Wire.beginTransmission(ADJD_S311_ADDRESS);
  Wire.write(address);
  Wire.write(data);
  Wire.endTransmission();
}

// read a byte of data from ADJD-S311 address
unsigned char readRegister(unsigned char address)
{
  unsigned char data;
  
  Wire.beginTransmission(ADJD_S311_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();
  
  Wire.requestFrom(ADJD_S311_ADDRESS, 1);
  while (!Wire.available())
    ;  // wait till we can get data
  
  return Wire.read();
}

// Write two bytes of data to ADJD-S311 address and addres+1
int readRegisterInt(unsigned char address)
{
  return readRegister(address) + (readRegister(address+1)<<8);
}
