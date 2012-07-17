/*
 * WiFlyHQ Example udpclient.ino
 *
 * This sketch implements a simple UDP client that sends a UDP packet
 * to a UDP server every second.
 *
 * This sketch is released to the public domain.
 *
 */


#include <SoftwareSerial.h>
#include <WiFlyHQ.h>

// Teensy can do regular Serial, so we might just
// want to swap it over, but we'll have to see
SoftwareSerial wifiSerial(10,11);

/* Change these to match your WiFi network */
const char mySSID[] = "hoembaes";
const char myPassword[] = "tigerstyle";

uint32_t lastSend = 0;
uint32_t count=0;

// this is the data packet
// transmitted like so:
// xx.xxx.xx.xxx:3000?id=THIS_ID&color=colorSensorVal
const int THIS_ID;
int colorSensorVal;

void terminal();

WiFly wifly;


void setup()
{
    char buf[32];

//    Serial.begin(115200);
//    //Serial.println("Starting");
//    //Serial.print("Free memory: ");
//    //Serial.println(wifly.getFreeMemory(),DEC);

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

    /* Join wifi network if not already associated */
    if (!wifly.isAssociated()) {
	/* Setup the WiFly to connect to a wifi network */
	//Serial.println("Joining network");
	wifly.setSSID(mySSID);
	wifly.setPassphrase(myPassword);
	wifly.enableDHCP();

	if (wifly.join()) {
	    //Serial.println("Joined wifi network");
	} else {
	    //Serial.println("Failed to join wifi network");
	    terminal();
	}
    } else {
        //Serial.println("Already joined network");
    }

    /* Ping the gateway */
    wifly.getGateway(buf, sizeof(buf));

    ////Serial.print("ping ");
    ////Serial.print(buf);
    ////Serial.print(" ... ");
    
    if (wifly.ping(buf)) {
	//Serial.println("ok");
    } else {
	//Serial.println("failed");
    }

    // Setup for UDP packets, sent automatically
    wifly.setIpProtocol(WIFLY_PROTOCOL_UDP);
    wifly.setHost("192.168.1.60", 8042);	// Send UDP packet to this server and port

    //Serial.print("MAC: ");
    //Serial.println(wifly.getMAC(buf, sizeof(buf)));
    //Serial.print("IP: ");
    //Serial.println(wifly.getIP(buf, sizeof(buf)));
    //Serial.print("Netmask: ");
    //Serial.println(wifly.getNetmask(buf, sizeof(buf)));
    //Serial.print("Gateway: ");
    //Serial.println(wifly.getGateway(buf, sizeof(buf)));

    wifly.setDeviceID("Wifly-UDP");
    //Serial.print("DeviceID: ");
    //Serial.println(wifly.getDeviceID(buf, sizeof(buf)));

    wifly.setHost("192.168.1.60", 8042);	// Send UPD packets to this server and port

    //Serial.println("WiFly ready");
}

void loop()
{
    if ((millis() - lastSend) > 1000) {
        count++;
	//Serial.print("Sending message ");
	//Serial.println(count);

	//wifly.print("Hello, count=");
	//wifly.println(count);
	lastSend = millis();
    }

//    if (Serial.available()) {
//        /* if the user hits 't', switch to the terminal for debugging */
//        if (Serial.read() == 't') {
//	    terminal();
//	}
//    }

}

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
