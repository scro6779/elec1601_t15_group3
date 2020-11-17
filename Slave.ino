//-----------------------------------------------------------------------------------------------------------//
//                                                                                                           //
//  Slave_ELEC1601_Student_2019_v3                                                                           //
//  The Instructor version of this code is identical to this version EXCEPT it also sets PIN codes           //
//  20191008 Peter Jones                                                                                     //
//                                                                                                           //
//  Bi-directional passing of serial inputs via Bluetooth                                                    //
//  Note: the void loop() contents differ from "capitalise and return" code                                  //
//                                                                                                           //
//  This version was initially based on the 2011 Steve Chang code but has been substantially revised         //
//  and heavily documented throughout.                                                                       //
//                                                                                                           //
//  20190927 Ross Hutton                                                                                     //
//  Identified that opening the Arduino IDE Serial Monitor asserts a DTR signal which resets the Arduino,    //
//  causing it to re-execute the full connection setup routine. If this reset happens on the Slave system,   //
//  re-running the setup routine appears to drop the connection. The Master is unaware of this loss and      //
//  makes no attempt to re-connect. Code has been added to check if the Bluetooth connection remains         //
//  established and, if so, the setup process is bypassed.                                                   //
//                                                                                                           //
//-----------------------------------------------------------------------------------------------------------//

#include <SoftwareSerial.h>   //Software Serial Port
#include <Servo.h>

#define RxD 7
#define TxD 6
#define ConnStatus A1
#define DEBUG_ENABLED  1

// ###################### Bluetooth Stuff ##############################

int shieldPairNumber = 3;
boolean ConnStatusSupported = true;   // Set to "true" when digital connection status is available on Arduino pin
// The following two string variable are used to simplify adaptation of code to different shield pairs
String slaveNameCmd = "\r\n+STNA=Slave";   // This is concatenated with shieldPairNumber later
SoftwareSerial blueToothSerial(RxD,TxD);

// #####################################################################

Servo servoLeft;
Servo servoRight;

int left_sensor_pin = 10;
int right_sensor_pin = 3;

int lState = 0;
int rState = 0;

int botSpeed = 100;
int TbotSpeed = 50;
int botDelay = 200;
int lOffset = 17;  //bigger is clockwise
int rOffset = 10;


bool botAuto = false;


void setup() {
  
    Serial.begin(9600);
    blueToothSerial.begin(38400);
    
    pinMode(RxD, INPUT);
    pinMode(TxD, OUTPUT);
    pinMode(ConnStatus, INPUT);
    pinMode(9, OUTPUT);
    pinMode(2, OUTPUT);
    pinMode(10, INPUT);
    pinMode(3, INPUT);


    //  Check whether Master and Slave are already connected by polling the ConnStatus pin (A1 on SeeedStudio v1 shield)
    //  This prevents running the full connection setup routine if not necessary.
    if(ConnStatusSupported) Serial.println("Checking Slave-Master connection status.");

    if(ConnStatusSupported && digitalRead(ConnStatus) == 1) Serial.println("Already connected to Master - remove USB cable if reboot of Master Bluetooth required.");
    else {
      
        Serial.println("Not connected to Master.");
        
        setupBlueToothConnection();   // Set up the local (slave) Bluetooth module

        delay(1000);                  // Wait one second and flush the serial buffers
        Serial.flush();
        blueToothSerial.flush();
      
    }
  
}

void stopRobot() {
  servoLeft.detach();
  servoRight.detach();
}

void loop() {
  
    char recvChar;
    
    // Receive
    if(blueToothSerial.available()) {
    
        recvChar = blueToothSerial.read();

        if (recvChar == 'z') {
          botAuto = !botAuto;

          if(botAuto) {
            botSpeed = 100;
            TbotSpeed = 70;
            botDelay = 50;
          } else {
            botSpeed = 100;
            TbotSpeed = 50;
            botDelay = 200;
          }
          
          servoLeft.detach();
          servoRight.detach();
          
        } else if(recvChar == 'o') {
          
          servoLeft.detach();
          servoRight.detach();
          botAuto = false;
          
        }
        
        if(!botAuto) {

          if(recvChar == 'w') moveRobot(0);
          else if(recvChar == 'd') moveRobot(1);
          else if(recvChar == 's') moveRobot(2);
          else if(recvChar == 'a') moveRobot(3);
          
        }
       
      
    }
    
    // Send
    if(Serial.available()) { // Send
    
        recvChar = Serial.read();
        Serial.println(recvChar);
        blueToothSerial.println(recvChar);

    }

    
    int lState = irDetect(9, 10, 38000);
    int rState = irDetect(2, 3, 38000);

//    blueToothSerial.println("Left: " + String(lState));
//    blueToothSerial.println("Right: " + String(rState));
      
    if(botAuto) {
    
      if(lState == 0 && rState == 1) {
        moveRobot(1); // 'd'
      } else if(lState == 1 && rState == 0) {
        moveRobot(3); // 'a'
      } else if(lState == 0 && rState == 0) {
        moveRobot(0); // 'w'
      }
//       else {
//        servoLeft.detach();
//        servoRight.detach();
//      }
      
    }

}

int irDetect(int irLedPin, int irReceiverPin, long frequency)
{
  tone(irLedPin, frequency, 8);            // IRLED 38 kHz for at least 1 ms
  delay(1);                               // Wait 1 ms
  int ir = digitalRead(irReceiverPin);    // IR receiver -> ir variable
  delay(1);                               // Down time before recheck
  return ir;                              // Return 1 no detect, 0 detect
}


void moveRobot(int dir) {

    servoLeft.attach(13);
    servoRight.attach(12);

    int leftVal = 0;
    int rightVal = 0;
    int delay_ = 0;
    String printStmnt = "";

    switch (dir) {

      case 0: // forward
        leftVal = 1500 - botSpeed;
        rightVal = 1500 + botSpeed - 9;
        delay_ = botDelay;
        printStmnt = "Forward";
        break;

      case 2: // back
        leftVal = 1500 + botSpeed - 9;
        rightVal = 1500 - botSpeed;
        delay_ = botDelay;
        printStmnt = "Backward";
        break;

      case 1: // right
        leftVal = 1500 + TbotSpeed;
        rightVal = 1500 + TbotSpeed - 9;
        delay_ = botDelay * 0.5;
        printStmnt = "Turning Right";
        break;

      case 3: // left
      
        leftVal = 1500 - TbotSpeed - 9;
        rightVal = 1500 - TbotSpeed;
        delay_ = botDelay * 0.5;
        printStmnt = "Turning Left";
        break;

    }

    delay(delay_);

    servoLeft.writeMicroseconds(leftVal +  lOffset);
    servoRight.writeMicroseconds(rightVal + rOffset);

    if (!botAuto) {
    servoLeft.detach();
    servoRight.detach();
    }
}
  

void setupBlueToothConnection() {
  
    Serial.println("Setting up the local (slave) Bluetooth module.");

    slaveNameCmd += shieldPairNumber;
    slaveNameCmd += "\r\n";

    blueToothSerial.print("\r\n+STWMOD=0\r\n");      // Set the Bluetooth to work in slave mode
    blueToothSerial.print(slaveNameCmd);             // Set the Bluetooth name using slaveNameCmd
    blueToothSerial.print("\r\n+STAUTO=0\r\n");      // Auto-connection should be forbidden here
    blueToothSerial.print("\r\n+STOAUT=1\r\n");      // Permit paired device to connect me
    
    //  print() sets up a transmit/outgoing buffer for the string which is then transmitted via interrupts one character at a time.
    //  This allows the program to keep running, with the transmitting happening in the background.
    //  Serial.flush() does not empty this buffer, instead it pauses the program until all Serial.print()ing is done.
    //  This is useful if there is critical timing mixed in with Serial.print()s.
    //  To clear an "incoming" serial buffer, use while(Serial.available()){Serial.read();}

    blueToothSerial.flush();
    delay(2000);                                     // This delay is required

    blueToothSerial.print("\r\n+INQ=1\r\n");         // Make the slave Bluetooth inquirable
    
    blueToothSerial.flush();
    delay(2000);                                     // This delay is required
    
    Serial.println("The slave bluetooth is inquirable!");
  
}
