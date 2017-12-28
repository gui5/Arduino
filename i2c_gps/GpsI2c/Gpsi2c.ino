
/*
    This sketch captures GPS data by serial, decodes and transmits pakets via i2c

    Developed by Guilherme de Oliveira MIT Licence
    https://github.com/gui5
*/

#include <Arduino.h>
#include <Wire.h>

//i2c adresses registers


#define GPS_ADDRESS (0x05)
#define MSG_1       ("$GPGGA")
#define MSG_2       ("$GPVTG")
#define INBUFF_SZ   (128)

typedef union
{
    float Float;
    unsigned long Long;
    byte Bytes[4];

} DATA_UNION_T;

enum GPS_DATA
{
    gps_TIME =0,
    gps_LAT,
    gps_LON,
    gps_HGT,
    gps_SPEED,
    gps_HEADING
};

DATA_UNION_T GpsData[6];

//Functions
bool Parse_msg1();
bool Parse_msg2();
bool SendData();
bool PrintOut();

//variables
char inStr[INBUFF_SZ];
byte uCharCount=0;
const u8 uHeader_size = strlen(MSG_1);
bool msg1_ok = false;
bool msg2_ok = false;
bool msg_ok =false;

void setup()
{
    Serial.begin(9600);
    Serial1.begin(9600);
    Wire.begin(GPS_ADDRESS);
    Wire.onRequest(requestEvent); // register event
}

void loop()
{   
    if(msg1_ok && msg2_ok)
    {
       
        PrintOut();
        msg1_ok = false;
        msg2_ok = false;
    }

    if (msg_ok==true)
    {
        if(!memcmp(inStr,MSG_1,uHeader_size))
        {
            Parse_msg1();
            msg1_ok = true;
           
        }

        else if(!memcmp(inStr,MSG_2,uHeader_size))
        {
            Parse_msg2();
            msg2_ok = true;
        }
        msg_ok = false;
    }

}

void serialEvent1()
{
    while(Serial1.available())
    {
        inStr[uCharCount] = Serial1.read();

        if( inStr[uCharCount]=='\n')
        {
            inStr[uCharCount+1]='\0';
            uCharCount = 0;
            msg_ok = true;
        } 
        else
        {
            uCharCount++;
        }
            
    }
}

void requestEvent() 
{
    for (unsigned int i=0;i<6;i++)
    {
        Wire.write(GpsData[i].Bytes,sizeof(GpsData[i].Bytes));
    }
}

bool Parse_msg1()
{
    char *data;
    int itCount=0;

    data = strtok(inStr,",");
    while (data != NULL)
    {
        switch(itCount)
        {
            case 1:
                GpsData[gps_TIME].Long = atol(data);
                break;
            case 2:
                GpsData[gps_LAT].Float = atol(data);
                break;
            case 3:
                if(data[0] == 'S')
                {
                    GpsData[gps_LAT].Float *= -1;
                }
                break;

            case 4:
                GpsData[gps_LON].Float = atof(data);
                break;

            case 5:
                if(data[0] == 'W')
                {
                    GpsData[gps_LON].Float *= -1;
                }
                break;

            case 9:
                GpsData[gps_HGT].Float = atof(data);
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
    char *data;
    int itCount=0;
    
    data = strtok(inStr,",");
   
    while (data != NULL)
    {
        switch(itCount)
        {
            case 1:
                GpsData[gps_HEADING].Float = atof(data);
                break;

            case 6:
                GpsData[gps_SPEED].Float = atof(data);
                break;

            default:
                break;
        }
        itCount++;
        data = strtok(NULL,",");
        
    }
    return false;
}
bool SendData()
{
   
    return false;
}

bool PrintOut()
{
    
    return false;
}
