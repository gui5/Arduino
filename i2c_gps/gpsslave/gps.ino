
#include <Wire.h>
#include <avr/wdt.h>

//slave address range: 0x01 - 0x7F
#define SLAVE_ADDRESS (0x29)

//register map
#define STATUS  (0x00)  //status register 0x1 = valid 0x00 = invalid
#define TIME    (0x01)  //time
#define LAT     (0x05)  //latitude 
#define LON     (0x09)  //longitude
#define HGT     (0x0D)  //hight
#define SPD     (0x11)  //speed
#define HED     (0x15)  //heading
#define MODE    (0x19)  //mode register
#define CONFIG  (0x1A)  //configuration register
#define IDR     (0x1B)  //identificaion register

#define REG_MAP_SZ      (0x1C)
#define MAX_SENT_BYTES  (0x03)

#define MSG_1       ("$GPGGA")
#define MSG_2       ("$GPVTG")

#define LED         (13)

char inBuf[128];

unsigned int count = 0;

const u8 uHeader_size = strlen(MSG_1);

typedef union GPS_DATA
{
    float f_float;
    unsigned long uLong;
    byte Bytes[3]; // 0-3
};

float fDegree =0.00f;
float fMinutes =0.00f;

GPS_DATA gdTime;        //unsigned long
GPS_DATA gdLatitude;    //float
GPS_DATA gdLongitude;   //float
GPS_DATA gdHeight;      //float
GPS_DATA gdSpeed;       //float
GPS_DATA gdHeading;     //float

bool new_msg = false;
bool msg1_ok = false;
bool msg2_ok = false;

int state = 0;

byte registerMap[REG_MAP_SZ];

byte receivedCommands[MAX_SENT_BYTES];

float getDecimal(float t_gpsData);

bool Parse_msg1();

bool Parse_msg2();

void setup()
{
    Wire.begin(SLAVE_ADDRESS);
    
    Wire.onRequest(requestEvent);

    Wire.onReceive(receiveEvent); 

    Serial.begin(9600); //setup serial for gps

    pinMode(LED, OUTPUT);

    wdt_enable(WDTO_1S);
   
}

void loop()
{
    wdt_reset();
    if (msg1_ok && msg2_ok)
    {
       registerMap[STATUS] = 0xff;
        digitalWrite(LED, (state) ? HIGH : LOW);
        state = !state;
        msg1_ok = false;
        msg2_ok = false;
    }
    
    if (new_msg)
    {
        if(!memcmp(inBuf,MSG_1,uHeader_size))
        {
            Parse_msg1();
            msg1_ok = true;
            wdt_reset();
           
        }
        else if(!memcmp(inBuf,MSG_2,uHeader_size))
        {
            Parse_msg2();
            msg2_ok = true;
            wdt_reset();
        }

        new_msg = false;
    }
        
}


//i2c events
void requestEvent()
{
    if(receivedCommands[0] < REG_MAP_SZ)
    {
        Wire.write(registerMap+receivedCommands[0], REG_MAP_SZ);
    }
    else
    {
        Wire.write(0x00);
    }
    
}
    
void receiveEvent(int bytesReceived)
{
    for (int a = 0; a < bytesReceived; a++)
    {
        if ( a < MAX_SENT_BYTES)
        {
            receivedCommands[a] = Wire.read();
        }
        else
        {
            Wire.read();  // if we receive more data then allowed just throw it away
        }
    
    }
}        

//serial events
void serialEvent()
{
    while(Serial.available())
    {
        inBuf[count] = Serial.read();

        if( inBuf[count]=='\n')
        {
            inBuf[count+1]='\0';
            count = 0;
            new_msg = true;
        } 
        else
        {
            count++;
        }
            
    }
}


//message parse
bool Parse_msg1()
{
    registerMap[STATUS] = 0;
    char *data;
    int itCount=0;

    data = strtok(inBuf,",");
    while (data != NULL)
    {
        switch(itCount)
        {
            case 1:
                gdTime.uLong = atol(data);
                registerMap[TIME] = gdTime.Bytes[3];
                registerMap[TIME+1] = gdTime.Bytes[2];
                registerMap[TIME+2] = gdTime.Bytes[1];
                registerMap[TIME+3] = gdTime.Bytes[0];
                break;
            case 2:
                
                gdLatitude.f_float = atof(data);
                gdLatitude.f_float = getDecimal( gdLatitude.f_float);
                break;
            case 3:
                if(data[0] == 'S')
                {
                    gdLatitude.f_float *= -1;
                    
                }
                registerMap[LAT] = gdLatitude.Bytes[3];
                registerMap[LAT+1] = gdLatitude.Bytes[2];
                registerMap[LAT+2] = gdLatitude.Bytes[1];
                registerMap[LAT+3] = gdLatitude.Bytes[0];
                break;

            case 4:
                gdLongitude.f_float = atof(data);
                gdLongitude.f_float = getDecimal( gdLongitude.f_float);
                break;

            case 5:
                if(data[0] == 'W')
                {
                    gdLongitude.f_float *= -1;
                }
                registerMap[LON] = gdLongitude.Bytes[3];
                registerMap[LON+1] = gdLongitude.Bytes[2];
                registerMap[LON+2] = gdLongitude.Bytes[1];
                registerMap[LON+3] = gdLongitude.Bytes[0];
                break;

            case 9:
               gdHeight.f_float = atof(data);
               registerMap[HGT] = gdHeight.Bytes[3];
               registerMap[HGT+1] = gdHeight.Bytes[2];
               registerMap[HGT+2] = gdHeight.Bytes[1];
               registerMap[HGT+3] = gdHeight.Bytes[0];
                break;

            default:
                break;
        }
        itCount++;
        data = strtok(NULL,",");
    }
    return false;
}

bool Parse_msg2()
{
    registerMap[STATUS] = 0;
    char *data;
    int itCount=0;
    
    data = strtok(inBuf,",");
   
    while (data != NULL)
    {
        switch(itCount)
        {
            case 1:
                gdHeading.f_float = atof(data);
                registerMap[HED] = gdHeading.Bytes[3];
                registerMap[HED+1] = gdHeading.Bytes[2];
                registerMap[HED+2] = gdHeading.Bytes[1];
                registerMap[HED+3] = gdHeading.Bytes[0];
                break;

            case 6:
               gdSpeed.f_float = atof(data);
               registerMap[SPD] = gdSpeed.Bytes[3];
               registerMap[SPD+1] = gdSpeed.Bytes[2];
               registerMap[SPD+2] = gdSpeed.Bytes[1];
               registerMap[SPD+3] = gdSpeed.Bytes[0];
                break;

            default:
                break;
        }
        itCount++;
        data = strtok(NULL,",");
        
    }
    return false;
}

float getDecimal(float t_gpsData)
{
    fDegree = int(t_gpsData) / 100;
    fMinutes = t_gpsData - 100*fDegree;
    return  (fDegree + fMinutes/60);
}
