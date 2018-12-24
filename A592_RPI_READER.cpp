//
//
#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <time.h>

/***********************************************************************/ 
/***********************************************************************/ 
RF24 radio( 25, 0 ); // RasPi
RF24Network network(radio);

const uint16_t this_node = 00;  // Address of our node
const uint16_t other_node = 01;  // Address of the other node

const unsigned long interval = 1000; // ms // Delay manager to send pings  regularly. 
unsigned long last_time_sent; 
static uint8_t sendType = 0; 

bool send_S(uint16_t to); // Prototypes for functions to send & handle messages bool send_D(uint16_t to); 
bool send_D(uint16_t to); 
void handle_S(RF24NetworkHeader& header); 
void handle_D(RF24NetworkHeader& header); 

typedef struct sensorTemperatureData 
{
    uint8_t id; // sensor id (1 - N_sensors)
    uint8_t status; // 0x80 = BATTERY LOW bit, 0x40 = Data Fresh bit,
    uint16_t temperature; // temperature value in C, no offset
    uint32_t timestamp; // number of seconds since startup
} sensorTemperatureData;

static sensorTemperatureData sensorData[6]; // I have 6 temperature sensors 

void setup() 
{  
	radio.begin();
	radio.setDataRate(RF24_250KBPS);
    radio.setPALevel(RF24_PA_MAX);
    radio.setCRCLength(RF24_CRC_8);
	radio.setRetries(7,7);
	delay(5);
	network.begin(/*channel*/ 90, /*node address*/ this_node);
	radio.printDetails();	
}

void loop() 
{
  network.update(); // Pump the network regularly
   while ( network.available() )
   { // Is there anything ready for us?
        RF24NetworkHeader header; // If so, take a look at it
        network.peek(header);
        printf( "\n\r---------------------------------\n\r");
        printf( header.toString() );
        switch (header.type)
        { // Dispatch the message to the correct handler.
            case 'S': handle_S(header); break;
            case 'D': handle_D(header); break;
            default: printf("*** WARNING *** Unknown message type %c\n\r",header.type);
                      network.read(header,0,0);
                      break;
        };
    }
    unsigned long now = millis(); // Send a ping every 'interval' ms
    if ( now > (last_time_sent + interval) )
    {
        last_time_sent = now;
        uint16_t to = 01; // Who should we send to? By default, send to base
        bool ok;
        sendType++;
        if ( sendType & 0x01 )
        {
            ok = send_S(to);
        }
        else
        {
            ok = send_D(to);
        }
        if (ok)
        { // Notify us of the result
            printf("%u: APP Send ok\n\r",millis());
        }
        else
        {
            printf("%u: APP Send failed\n\r",millis());
        }
    }
}
/**
 *
 */ 
 bool send_S(uint16_t to) 
 {
    RF24NetworkHeader header(/*to node*/ to, /*type*/ 'S' /*Status*/);
    // The 'S' message
    printf("\n\r---------------------------------\n\r");
    printf("Sending %c to 0%o...\n\r", 'S', to);
    return network.write(header, 0, 0);
}
/**
 *
 */ 
bool send_D(uint16_t to) 
 {
    static uint8_t nodeId = 1; // which node id to ask for
    
    RF24NetworkHeader header(/*to node*/ to, /*type*/ 'D' /*Data*/);
    // The 'D' message
    uint8_t cmd = nodeId;
    printf("\n\r---------------------------------\n\r");
    printf("Sending %c id %u to 0%o...\n\r", 'D', nodeId, to);
    bool rsp = network.write(header,&cmd,sizeof(cmd));
    
    if( (nodeId++) > 6 )
    {
        nodeId = 1;
    }
    
    return rsp;
}
/**
 * Receive and display the sensor status bytes
 *
 */ 
 void handle_S(RF24NetworkHeader& header) 
 {
    // The 'S' message is status of all sensor nodes
    uint8_t statusArray[ 6 ]; // I have 6 temperature sensors in my house
    int numread = network.read(header,&statusArray,sizeof(statusArray));
    
    //for( int i = 0; i < 6; i++ )
    //{
    // printf_P(PSTR("Received %02X from 0%o\n\r"), statusArray[i],header.from_node);
    //}
    
    printf("\n\r");
    printf("STATUS: ");
    printf("%02X ", statusArray[0] );
    printf("%02X ", statusArray[1] );
    printf("%02X ", statusArray[2] );
    printf("%02X ", statusArray[3] );
    printf("%02X ", statusArray[4] );
    printf("%02X ", statusArray[5] );
    printf("\n\r");
}
/**
 * Receive and display a sensor reading
 */ 
 void handle_D(RF24NetworkHeader& header) 
 {
    // The 'D' message is data
    sensorTemperatureData sensorData;
    int numread = network.read(header,&sensorData,sizeof(sensorData));
    
    printf("\n\r");
    printf("id = %u", sensorData.id);
    printf(", status = 0x%02X", sensorData.status);
    printf(", temperature = %d", sensorData.temperature);
    printf(", temperature = %5.2f", (sensorData.temperature*0.18)+32);
    printf(", time = %u", sensorData.timestamp);
    printf("\n\r");
}

int main(int argc, char** argv) 
{
	setup();
	
	while(1)
	{
		loop();
	}
}


