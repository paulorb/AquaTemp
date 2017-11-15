#include <Wire.h>

void setup() {
   Serial.begin(9600);
  Serial.println("Iniciando teste"); 
  Wire.begin(); // join i2c bus (address optional for master)
}

void loop() {
Menu();
}

//
//byte device = 7;
//byte error = 0;
//byte x = 0x70;
//byte y = 0x0;
//byte numBytes = 0;
//void loop() {
//
// if(device<120) {
//    device = device +1;
//  }else{
//    device= 7;
//  }
//  
//
//  Wire.beginTransmission(device); // transmit to device #8
// Wire.write("ATP");              // sends one byte
//  error = Wire.endTransmission();    // stop transmitting
//
// delayMicroseconds(5000); //needs at least 1.3us free time between start and stop
//
// 
//
//  
//  if(error == 0){
//     Serial.println("OK! ");
//     Serial.print(device, DEC);
//     Serial.println("Request from");
//     numBytes =  Wire.requestFrom(device, 3);
//     if(numBytes == 0) {
//        numBytes =  Wire.requestFrom(device, 3);
//     }
//     Serial.print(numBytes, DEC);
//     while(Wire.available())
//      {
//          Serial.println("recebi!");
//          y = Wire.read();
//      }
//
//  }
//  if(error == 1){
//     Serial.println("Data too long!");
//  }
//  if(error == 2){
//   //  Serial.println("NACK Address!");
//  }
//  if(error == 3){
//     Serial.println("NACK Data!");
//  }
//  if(error == 4){
//     Serial.println("Other error!");
//  }
//
////  x++;
//  delay(5);
//}


void Browse() {

  byte error = 0;
  int i=0;
  for(i=8;i<127;i++){
     Wire.beginTransmission(i); // transmit to device #8
     Wire.write("0");              // sends one byte
     error = Wire.endTransmission();    // stop transmitting
     if(error == 0){
     Serial.println("Device found:");
     Serial.print(i, DEC);
    }
     if(error == 1){
        Serial.println("Data too long!");
     }
    if(error == 2){
      //  Serial.println("NACK Address!");
    }
    if(error == 3){
      Serial.println("NACK Data!");
     }
    if(error == 4){
       Serial.println("Other error!");
    }
  }
}

int GetInt() {
 while(Serial.available() <=0)
  {
    
  }
  return Serial.parseInt();
}

void Ping() {
  byte localReceive[10];
  int index =0;
  byte error = 0;
  int numBytes=0;
  Serial.println("Type the device id to ping: ");
  int deviceID  = GetInt();
  Wire.beginTransmission(deviceID); // transmit to device #8
  Wire.write("AT");              // sends one byte
  Wire.write(0x1);
  error = Wire.endTransmission();    // stop transmitting

  if(error == 0){
     numBytes =  Wire.requestFrom(deviceID, 3);
     if(numBytes == 0) {
        numBytes =  Wire.requestFrom(deviceID, 3);
     }
     if(numBytes == 0){
      Serial.println("None bytes received");
      return;
     }
     while(Wire.available())
      {
          localReceive[index++] = Wire.read();
      }
      Serial.println("Number of Bytes received:");
      Serial.print(numBytes, DEC);
      Serial.println("\n");
      Serial.print(localReceive[0], HEX);
      Serial.println("\n");
      Serial.print(localReceive[1], HEX);
      Serial.println("\n");
      Serial.print(localReceive[2], HEX);
      Serial.println("\n");
      if(localReceive[0] == 'A' && localReceive[1] == 'S' && localReceive[2] == 0x1) {
         Serial.println("PONG received");
      }
      
  }
  if(error == 1){
     Serial.println("ERROR: Data too long!");
  }
  if(error == 2){
     Serial.println("ERROR: NACK Address!");
  }
  if(error == 3){
     Serial.println("ERROR: NACK Data!");
  }
  if(error == 4){
     Serial.println("ERROR: Other error!");
  }
}

void PCFan() {
   byte localReceive[10];
  int index =0;
  byte error = 0;
  int numBytes=0;
  Serial.println("Type the device id to control FAN: ");
  int deviceID  = GetInt();
  Serial.println("Type 1 to turn on and 0 to turn off the FAN: ");
  int statusFan  = GetInt();
  
  Wire.beginTransmission(deviceID); // transmit to device #8
  Wire.write('A');              // sends one byte
  Wire.write('T');              // sends one byte
  Wire.write(0x06);              // sends one byte
  if(statusFan == 1){
    Wire.write(0x01);              // sends one byte
  }else{
    Wire.write(0x00);              // sends one byte
  }
  error = Wire.endTransmission();    // stop transmitting

  if(error == 0){
     numBytes =  Wire.requestFrom(deviceID, 4);
     if(numBytes == 0) {
        numBytes =  Wire.requestFrom(deviceID, 4);
     }
     if(numBytes == 0){
      Serial.println("None bytes received");
      return;
     }
     while(Wire.available())
      {
          localReceive[index++] = Wire.read();
      }
      if(localReceive[0] == 'A' && localReceive[1] == 'S' && localReceive[2] == 0x6 && localReceive[3] == 0xFF ) {
         Serial.println("ACK received, FAN Control Success");
      }
      
  }
  if(error == 1){
     Serial.println("ERROR: Data too long!");
  }
  if(error == 2){
     Serial.println("ERROR: NACK Address!");
  }
  if(error == 3){
     Serial.println("ERROR: NACK Data!");
  }
  if(error == 4){
     Serial.println("ERROR: Other error!");
  }
}

void ReadPCFanRPM() {
   byte localReceive[10];
  int index =0;
  byte error = 0;
  int numBytes=0;
  Serial.println("Type the device id to read FAN: ");
  int deviceID  = GetInt();

  
  Wire.beginTransmission(deviceID); // transmit to device #8
  Wire.write('A');              // sends one byte
  Wire.write('T');              // sends one byte
  Wire.write(0x03);              // sends one byte
  error = Wire.endTransmission();    // stop transmitting

  if(error == 0){
     numBytes =  Wire.requestFrom(deviceID, 7);
     if(numBytes == 0) {
        numBytes =  Wire.requestFrom(deviceID, 7);
     }
     if(numBytes == 0){
      Serial.println("None bytes received");
      return;
     }
     while(Wire.available())
      {
          localReceive[index++] = Wire.read();
      }
      Serial.println("Number of Bytes received:");
      Serial.print(numBytes, DEC);
      if(localReceive[0] == 'A' && localReceive[1] == 'S' && localReceive[2] == 0x3) {
        int rpm =0;
        Serial.print(localReceive[3], HEX);
        Serial.print(localReceive[4], HEX);
        Serial.print(localReceive[5], HEX);
        Serial.print(localReceive[6], HEX);
         Serial.println("RPM Received");
      }
      
  }
  if(error == 1){
     Serial.println("ERROR: Data too long!");
  }
  if(error == 2){
     Serial.println("ERROR: NACK Address!");
  }
  if(error == 3){
     Serial.println("ERROR: NACK Data!");
  }
  if(error == 4){
     Serial.println("ERROR: Other error!");
  }
}



void ReadTemperatureSensor(){
   byte localReceive[10];
  int index =0;
  byte error = 0;
  int numBytes=0;
  Serial.println("Type the device id to read Temperature: ");
  int deviceID  = GetInt();

  
  Wire.beginTransmission(deviceID); // transmit to device #8
  Wire.write('A');              // sends one byte
  Wire.write('T');              // sends one byte
  Wire.write(0x02);              // sends one byte
  error = Wire.endTransmission();    // stop transmitting

 if(error == 0){
     numBytes =  Wire.requestFrom(deviceID, 7);
     if(numBytes == 0) {
        numBytes =  Wire.requestFrom(deviceID, 7);
     }
     if(numBytes == 0){
      Serial.println("None bytes received");
      return;
     }
     while(Wire.available())
      {
          localReceive[index++] = Wire.read();
      }
      Serial.println("Number of Bytes received:");
      Serial.print(numBytes, DEC);
      if(localReceive[0] == 'A' && localReceive[1] == 'S' && localReceive[2] == 0x2) {
        int rpm =0;
        Serial.print(localReceive[3], HEX);
        Serial.print(localReceive[4], HEX);
        Serial.print(localReceive[5], HEX);
        Serial.print(localReceive[6], HEX);
         Serial.println("Temp Received");
      }else{
        if(localReceive[0] == 'A' && localReceive[1] == 'E' && localReceive[2] == 0x1) {
          Serial.println("No sensor found!");
        }else{
          Serial.println("Other error!");
          Serial.print(localReceive[0], HEX);
        Serial.print(localReceive[1], HEX);
        Serial.print(localReceive[2], HEX);
           Serial.print(localReceive[3], HEX);
        Serial.print(localReceive[4], HEX);
        Serial.print(localReceive[5], HEX);
        Serial.print(localReceive[6], HEX);
        }
      }
      
  }
  if(error == 1){
     Serial.println("ERROR: Data too long!");
  }
  if(error == 2){
     Serial.println("ERROR: NACK Address!");
  }
  if(error == 3){
     Serial.println("ERROR: NACK Data!");
  }
  if(error == 4){
     Serial.println("ERROR: Other error!");
  }
  
}

void PeltierControl() {
 byte localReceive[10];
  int index =0;
  byte error = 0;
  int numBytes=0;
  Serial.println("Type the device id to control Peltier: ");
  int deviceID  = GetInt();
  Serial.println("Type 1 to turn on and 0 to turn off the Peltier: ");
  int statusFan  = GetInt();
  
  Wire.beginTransmission(deviceID); // transmit to device #8
  Wire.write('A');              // sends one byte
  Wire.write('T');              // sends one byte
  Wire.write(0x07);              // sends one byte
  if(statusFan == 1){
    Wire.write(0x01);              // sends one byte
  }else{
    Wire.write(0x00);              // sends one byte
  }
  error = Wire.endTransmission();    // stop transmitting

  if(error == 0){
     numBytes =  Wire.requestFrom(deviceID, 4);
     if(numBytes == 0) {
        numBytes =  Wire.requestFrom(deviceID, 4);
     }
     if(numBytes == 0){
      Serial.println("None bytes received");
      return;
     }
     while(Wire.available())
      {
          localReceive[index++] = Wire.read();
      }
      if(localReceive[0] == 'A' && localReceive[1] == 'S' && localReceive[2] == 0x7 && localReceive[3] == 0xFF ) {
         Serial.println("ACK received, FAN Control Success");
      }else{
        Serial.println("Error!");
         Serial.print(localReceive[0], HEX);
        Serial.print(localReceive[1], HEX);
        Serial.print(localReceive[2], HEX);
           Serial.print(localReceive[3], HEX);
      }
      
  }
  if(error == 1){
     Serial.println("ERROR: Data too long!");
  }
  if(error == 2){
     Serial.println("ERROR: NACK Address!");
  }
  if(error == 3){
     Serial.println("ERROR: NACK Data!");
  }
  if(error == 4){
     Serial.println("ERROR: Other error!");
  }
}




char GetChar(){
  while(Serial.available() <=0)
  {
    
  }
  return Serial.read();
}

void Menu(){
  Serial.println("\nAquaArduino - V1 PauloRB");
  Serial.println("Type one of the following commands:");
  Serial.println("1 - Browse for devices");
  Serial.println("2 - Ping specific device");
  Serial.println("3 - Turn On/Off PC Fan");
  Serial.println("4 - Read PC Fan RPM");
  Serial.println("5 - Read Temperature Sensor");
  Serial.println("6 - Turn On/Off Peltier");
  char menu = GetChar();
  switch(menu) {
  case '1':
    Browse();
  break;
  case '2':
    Ping();
  break;
  case '3':
    PCFan();
   break;
   case '4':
     ReadPCFanRPM();
   break;
   case '5':
    ReadTemperatureSensor();
   break;
   case '6':
   PeltierControl();
   break;
  }
}
  




