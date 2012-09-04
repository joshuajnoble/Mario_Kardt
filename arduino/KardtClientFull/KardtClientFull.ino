//////////////////////////////////////////////////////////////////
// Motor
//////////////////////////////////////////////////////////////////

#define PWMA 10
#define PWMB 9
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


#include <SoftwareSerial.h> // need this to talk to the wifly shield.
#include <WiFlyHQ.h> // wifly!
#include <Wire.h> // i2c library to talk to the color sensor

// ADJD-S311 Pin definitions:
int sdaPin = 5;  // serial data, hardwired, can't change
int sclPin = 6;  // serial clock, hardwired, can't change
int ledPin = 2;  // LED light source pin, any unused pin will work

// initial values for integration time registers
unsigned char colorCap[4] = {9, 9, 2, 5};  // values must be between 0 and 15
unsigned int colorInt[4] = {2048, 2048, 2048, 2048};  // max value for these is 4095
unsigned int colorData[4];  // This is where we store the RGB and C data values
signed char colorOffset[4];  // Stores RGB and C offset values

//////////////////////////////////////////////////////////////////
// WIFLY
//////////////////////////////////////////////////////////////////

bool hasGottenIDFromServer = false;

// Teensy can do regular Serial, so we might just
// want to swap it over, but we'll have to see
SoftwareSerial wifiSerial(10,11);

// Change these to match your WiFi network
const char mySSID[] = "hoembaes";
const char myPassword[] = "";

uint32_t lastSend = 0;
uint32_t count=0;

const int PORT = 8042;
const char IP[] = "192.168.1.60";

// this is the data packet
// transmitted like so:
// xx.xxx.xx.xxx:3000?id=THIS_ID&color=colorSensorVal
// xx.xxx.xx.xxx:3000?id=THIS_ID&color=1
int THIS_ID;
int colorSensorVal;

void terminal();
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
uint16_t left, right;

void setup()
{


  // set up the motor driver

  pinMode(PWMA,OUTPUT);
  pinMode(AIN1,OUTPUT);
  pinMode(AIN2,OUTPUT);
  pinMode(PWMB,OUTPUT);
  pinMode(BIN1,OUTPUT);
  pinMode(BIN2,OUTPUT);
  pinMode(STBY,OUTPUT);

  char buf[32];

  /////////////////////////////////////////////////////
  // WIFLY
  /////////////////////////////////////////////////////

  wifiSerial.begin(9600);

  if (!wifly.begin(&wifiSerial, &Serial)) {
    //Serial.println("Failed to start wifly");
    terminal();
  }

  if (wifly.getFlushTimeout() != 10) {
    //Serial.println("Restoring flush timeout to 10msecs");
    wifly.setFlushTimeout(10);
    wifly.save();
    wifly.reboot();
  }

  // Join wifi network if not already associated
  if (!wifly.isAssociated()) {
    /* Setup the WiFly to connect to a wifi network */
    //Serial.println("Joining network");
    wifly.setSSID(mySSID);
    wifly.setPassphrase(myPassword);
    wifly.enableDHCP();

    if (wifly.join()) {
      //Serial.println("Joined wifi network");
    } 
    else {
      //Serial.println("Failed to join wifi network");
      terminal();
    }
  } else {
    //Serial.println("Already joined network");
  }

  /* Ping the gateway */
  wifly.getGateway(buf, sizeof(buf));

  if (wifly.ping(buf)) {
    //Serial.println("ok");
  } 
  else {
    //Serial.println("failed");
  }

  // Setup for UDP packets, sent automatically
  wifly.setIpProtocol(WIFLY_PROTOCOL_UDP);
  // do we need device ID before setHost()?
  wifly.setHost(IP, PORT);	// Send UDP packet to this server and port
  wifly.setDeviceID("Wifly-UDP");

  ////////////////////////////////////////////////////
  // COLOR SENSOR

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

}


//
void loop()
{
  // check wifly
  if(!hasGottenIDFromServer) {
    wifly.println("GET /id");
    
    // just wait for this
    while( wifly.available() < 0 ) {
    }
    
    THIS_ID = wifly.read();
    hasGottenIDFromServer = true;
    
  } else {
    
    // check the ADJD
    getRGBC();
    
    int color = checkColors();
    
    if( color != -1 && !lastReqResponded ) {
      wifly.print("GET /play?id=");
      wifly.print(THIS_ID);
      wifly.print('&color=');
      wifly.println(color);
      lastReqResponded = false;
    } else if( !lastReqResponded ) {
      
      wifly.print("GET /play?id=");
      wifly.print(THIS_ID);
      lastReqResponded = false;
    }
    
    char t[3];
    memset(t, 0x20, 3);
    
    int ind;
    
    // wait for all 7 chars
    if(wifly.available() > 7) {
      lastReqResponded = true;

      boolean leading = true;
      int i = 0;
      while( i < 3 ) {
        char c = (char) wifly.read();
        if(c != '0'){
          leading = false;
        }
        if(!leading) {
          t[i] = c;
        }
        i++;
      }
      char sep = wifly.read(); // throw away
      
      left = (int) strtol(&t[0], NULL, 0);
      leading = true;
      
      int i = 0;
      while( i < 3 ) {
        char c = (char) wifly.read();
        if(c != '0'){
          leading = false;
        }
        if(!leading) {
          t[i] = c;
        }
        i++;
      }
      
      right = (int) strtol(&t[0], NULL, 0);
    }
    
    //void motor_control(char motor, char direction, unsigned char speed)
    if(left > 255) {
      motor_control( MOTOR_A, FORWARD, left - 255);
    } else {
      motor_control( MOTOR_A, BACKWARD, 255 - left);
    }
    
    if(right > 255) {
      motor_control( MOTOR_B, FORWARD, right - 255);
    } else {
      motor_control( MOTOR_B, BACKWARD, 255 - right);
    }
    
  }
}

int checkColors()
{
  
  // find the color we need
  for( int i = 0; i < 6; i++) {
    if( abs(colors[i][0] - COLORS[RED] ) < 20 ) {
      if( abs(colors[i][1] - COLORS[GREEN] ) < 20 ) {
        if( abs(colors[i][2] - COLORS[BLUE] ) < 20 ) {
          if( abs(colors[i][3] - COLORS[CLEAR] ) < 20 ) {
            return index;
          }
        }
      }
    }
  }
  
  return -1;
}

//////////////////////////////////////////////////////////////////
// WIFLY
//////////////////////////////////////////////////////////////////

void terminal()
{
  //Serial.println("Terminal ready");
  while (1) {
    if (wifly.available() > 0) {
      Serial.write(wifly.read());
    }

    if (Serial.available()) {
      wifly.write(Serial.read());
    }
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

//Stops the motors from spinning and locks the wheels
void motor_brake()
{
  digitalWrite(AIN1,1);
  digitalWrite(AIN2,1);
  digitalWrite(PWMA,LOW);
  digitalWrite(BIN1,1);
  digitalWrite(BIN2,1);
  digitalWrite(PWMB,LOW);
} 

//Controls the direction the motors turn, speed from 0(off) to 255(full speed)
void motor_drive(char direction, unsigned char speed)
{
  if (direction == FORWARD)
  {
    motor_control(MOTOR_A, FORWARD, speed);
    motor_control(MOTOR_B, FORWARD, speed);
  }
  else
  {
    motor_control(MOTOR_A, REVERSE, speed);
    motor_control(MOTOR_B, REVERSE, speed);
  }
}

//You can control the turn radius by specifying the speed of each motor
//Set both to 255 for it to spin in place
void motor_turn(char direction, unsigned char speed_A, unsigned char speed_B )
{
  if (direction == RIGHT)
  {
    motor_control(MOTOR_A, REVERSE, speed_A);
    motor_control(MOTOR_B, FORWARD, speed_B);
  }
  else
  {
    motor_control(MOTOR_A, FORWARD, speed_A);
    motor_control(MOTOR_B, REVERSE, speed_B);
  }
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

/* I2C functions...*/
// Write a byte of data to a specific ADJD-S311 address
void writeRegister(unsigned char data, unsigned char address)
{
  
  //Serial.println( " calling writeRegister ");
  Wire.beginTransmission(ADJD_S311_ADDRESS);
  //Serial.println( " beginTrans ");
  Wire.write(address);
  //Serial.println( " write(address) ");
  Wire.write(data);
  //Serial.println( " write(data) ");
  
  Wire.endTransmission();
  //Serial.println( " leaving writeRegister ");
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
