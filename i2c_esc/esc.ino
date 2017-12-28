
#include <Arduino.h>
#include <Wire.h>
#include <ESC.h> //https://www.robotshop.com/blog/en/rc-speed-controller-esc-arduino-library-20470

#define I2C_ADDRESS     (0x20)  //slave address, any number from 0x01 to 0x7F
#define REG_MAP_SZ      (8)
#define MAX_SENT_BYTES  (3)

#define ESC_1_PIN   (3)
#define ESC_2_PIN   (5)
#define ESC_3_PIN   (6)
#define ESC_4_PIN   (9)

/*
    1 = true
    0 = false

                        CONFIG REGISTER
    BIT7 | BIT6 | BIT5 | BIT4 |  BIT3  |  BIT2  |  BIT1  |  BIT0
      x      x     x      x      esc4_en  esc3_en  esc2_en  esc1_en

                            MODE
    BIT7 | BIT6 | BIT5 | BIT4 |  BIT3  |  BIT2  |  BIT1  |  BIT0
      x      x     x       x      x       cal     test      run

                            STATUS
     BIT7 | BIT6 | BIT5 | BIT4 |  BIT3  |  BIT2  |  BIT1  |  BIT0

*/

enum BIT_MASK
{
    BIT0 = 1,
    BIT1 = 2,
    BIT2 = 4,
    BIT3 = 8,
    BIT4 = 16,
    BIT5 = 32,
    BIT6 = 64,
    BIT7 = 128

};

//reguster map enum
enum class REG_MAP
{
    STATUS, //read only
    ESC_1_THROTTLE,
    ESC_2_THROTTLE,
    ESC_3_THROTTLE,
    ESC_4_THROTTLE,
    MODE,
    CONFIG,
    IDENTIFICATION //read only
};


enum RCV_BYTE
{
    REGISTER,
    DATA,
    EXTRA
};

byte registerMap[REG_MAP_SZ];
byte receivedCommands[MAX_SENT_BYTES];
bool updateEsc[4];


//functions foot print
void setConfig(byte data);
void setMode(byte data);
void setThrottle(byte esc, byte value);
int getSpeed(byte value);
void calEsc();

ESC Esc1(ESC_1_PIN,1000,2000,500);
ESC Esc2(ESC_2_PIN,1000,2000,500);
ESC Esc3(ESC_3_PIN,1000,2000,500);
ESC Esc4(ESC_4_PIN,1000.2000,500);

void setup()
{
    Wire.begin(I2C_ADDRESS);

    Wire.onRequest(requestEvent);

    Wire.onReceive(receiveEvent);

    memset(registerMap,0,REG_MAP_SZ);

    registerMap[(int)REG_MAP::IDENTIFICATION] = I2C_ADDRESS;

    Serial.begin(115200); //debug

    delay(1000);

    Serial.print("ESC control module, address: 0x");
    Serial.println(I2C_ADDRESS,HEX);
}

void loop()
{
   
    if ( registerMap[(int)REG_MAP::MODE] & BIT2)
    {
        registerMap[(int)REG_MAP::MODE] &= ~BIT2; //reset calib bit
        calEsc();
    }

  if(updateEsc[0]==true && (registerMap[(int)REG_MAP::CONFIG] & BIT0))
  {
    Esc1.speed(getSpeed(registerMap[(int)REG_MAP::ESC_1_THROTTLE]));
    updateEsc[0] = false;
  }
  
  if(updateEsc[1]==true && (registerMap[(int)REG_MAP::CONFIG] & BIT1))
  {
    Esc2.speed(getSpeed(registerMap[(int)REG_MAP::ESC_2_THROTTLE]));
    updateEsc[1] = false;
  }

  if(updateEsc[2]==true && (registerMap[(int)REG_MAP::CONFIG] & BIT2))
  {
    Esc3.speed(getSpeed(registerMap[(int)REG_MAP::ESC_3_THROTTLE]));
    updateEsc[2] = false;
  }

  if(updateEsc[3]==true && (registerMap[(int)REG_MAP::CONFIG] & BIT3))
  {
    Esc4.speed(getSpeed(registerMap[(int)REG_MAP::ESC_4_THROTTLE]));
    updateEsc[3] = false;
  }

}

void requestEvent()
{
   
    Wire.write(registerMap + receivedCommands[0], REG_MAP_SZ); 
 
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

    if (bytesReceived>1) //needs at least rcv more than one byte ro set a register; register, value;
    {

        if(receivedCommands[REGISTER] == (int)REG_MAP::ESC_1_THROTTLE || receivedCommands[REGISTER] == (int)REG_MAP::ESC_2_THROTTLE || receivedCommands[REGISTER] == (int)REG_MAP::ESC_3_THROTTLE || receivedCommands[REGISTER] == (int)REG_MAP::ESC_4_THROTTLE)
        {
            setThrottle(receivedCommands[REGISTER],receivedCommands[DATA]);
            return;
        }

        if(receivedCommands[REGISTER] == (int)REG_MAP::STATUS || receivedCommands[REGISTER] == (int)REG_MAP::IDENTIFICATION)
        {
            Serial.println("ERROR: read only register");
            return;
        }

        if(receivedCommands[REGISTER] == (int)REG_MAP::CONFIG)
         {
             setConfig(receivedCommands[DATA]);
             return;
         }

        if(receivedCommands[REGISTER] == (int)REG_MAP::MODE)
        {
            setMode(receivedCommands[DATA]);
            return;
        }
    }
}


void setConfig(byte data)
{

   if (data & BIT0)
   {
      
       Serial.println("ESC_1: ENABLED");
   }
   else
   {
    Serial.println("ESC_1: DISABLED");
   }

   if (data & BIT1)
   {
       
       Serial.println("ESC_2: ENABLED");
   }
   else
   {
       
    Serial.println("ESC_2: DISABLED");
   }

   if (data & BIT2)
   {
       
       Serial.println("ESC_3: ENABLED");
   }
   else
   {
      
    Serial.println("ESC_3: DISABLED");
   }

   if (data & BIT3)
   {
      
       Serial.println("ESC_4: ENABLED");
   }
   else
   {
      
    Serial.println("ESC_4: DISABLED");
   }


    registerMap[(int)REG_MAP::CONFIG] = data;

}

void setMode(byte data)
{
    if(data & BIT0)
    {
        Serial.println("Mode: RUN");
        if(registerMap[(int)REG_MAP::CONFIG] & BIT0)
        {
            Esc1.arm();
        }

        if(registerMap[(int)REG_MAP::CONFIG] & BIT1)
        {
            Esc2.arm();
        }

        if(registerMap[(int)REG_MAP::CONFIG] & BIT2)
        {
            Esc3.arm();
        }

        if(registerMap[(int)REG_MAP::CONFIG] & BIT3)
        {
            Esc4.arm();
        }


    }
    else
    {
        Serial.println("Mode: STOP");
        Esc1.stop();
        Esc2.stop();
        Esc3.stop();
        Esc4.stop();
    }

    if(data & BIT1)
    {
        Serial.println("Mode: TEST");
    }

    if(data & BIT2)
    {
        Serial.println("Mode: CAL");
    }

    registerMap[(int)REG_MAP::MODE] = data;
}

void setThrottle(byte esc, byte value)
{
    if (esc == (int)REG_MAP::ESC_1_THROTTLE)
    {
        registerMap[(int)REG_MAP::ESC_1_THROTTLE] = value;
        updateEsc[0] = true;
        return;
    }

    if (esc == (int)REG_MAP::ESC_2_THROTTLE)
    {
        registerMap[(int)REG_MAP::ESC_2_THROTTLE] = value;
        updateEsc[1] = true;
        return;
    }

    if (esc == (int)REG_MAP::ESC_1_THROTTLE)
    {
        registerMap[(int)REG_MAP::ESC_2_THROTTLE] = value;
        updateEsc[2] = true;
        return;
    }

    if (esc == (int)REG_MAP::ESC_2_THROTTLE)
    {
        registerMap[(int)REG_MAP::ESC_2_THROTTLE] = value;
        updateEsc[3] = true;
        return;
    }
}

int getSpeed(byte value)
{

     return map(value, 0, 255, 1000, 2000);

}

void calEsc()
{
   
      Esc1.calib();
      Esc1.stop();
   
      Esc2.calib();
      Esc2.stop();
  
      Esc3.calib();
      Esc3.stop();
  
      Esc4.calib();
      Esc4.stop();
   

    Serial.println("ESC calib complete.");
}