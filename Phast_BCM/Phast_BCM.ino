const char* version = "Phast v1.2.0";
const char* company = "TanrTech";

#include <AFMotor.h>
#include <SoftwareSerial.h>

// Pin setup
int const relayPin = A0;
int const Rled = A1;
int const Gled = A2;
int const Bled = A3;
int const Rx = A4;
int const Tx = A5;

// Motor controller setup
AF_DCMotor valve(4);
#define OPEN BACKWARD
#define CLOSE FORWARD

// Link to ECU
SoftwareSerial link(Rx, Tx);
char cString[20];
byte chPos = 0;
byte ch = 0;

void (*sysReset)(void) = 0;

void setup() {
  Serial.begin(9600);

  // Initialize relay
  pinMode(relayPin, OUTPUT);
  relayOFF();

  // Initialize valve
  valve.setSpeed(255);
  valveCLOSE();

  // Initialize LED
  pinMode(Rled, OUTPUT);
  pinMode(Gled, OUTPUT);
  pinMode(Bled, OUTPUT);

  // Initialize link to ECU
  pinMode(Rx, INPUT);
  pinMode(Tx, OUTPUT);
  link.begin(9600);
}

void loop() {
  while (link.available() > 0) {
    ch = link.read();
    cString[chPos] = ch;
    chPos++;
    delay(1);
  }
  cString[chPos] = 0;  //terminate cString
  chPos = 0;
  Serial.print(cString);

  if (strcmp(cString, "SYN") == 0) {
    link.print(version);
    relayOFF();
    valveCLOSE();
    ledOFF();
  } else if (strcmp(cString, "VOP") == 0) {
    link.print("ACK");
    valveOPEN();
  } else if (strcmp(cString, "RON") == 0) {
    link.print("ACK");
    relayON();
  } else if (strcmp(cString, "VCL") == 0) {
    link.print("ACK");
    valveCLOSE();
  } else if (strcmp(cString, "ROF") == 0) {
    link.print("ACK");
    relayOFF();
  } else if (strcmp(cString, "LRD") == 0) {
    link.print("ACK");
    ledRED();
  } else if (strcmp(cString, "LGN") == 0) {
    link.print("ACK");
    ledGREEN();
  } else if (strcmp(cString, "LBL") == 0) {
    link.print("ACK");
    ledBLUE();
  } else if (strcmp(cString, "LOF") == 0) {
    link.print("ACK");
    ledOFF();
  } else if (strcmp(cString, "RST") == 0) {
    link.print("ACK");
    sysReset();
  }
}

void relayOFF() {
  digitalWrite(relayPin, LOW);
}

void relayON() {
  digitalWrite(relayPin, HIGH);
}

void valveCLOSE() {
  valve.run(CLOSE);
  delay(30);
  valve.run(RELEASE);
}

void valveOPEN() {
  valve.run(OPEN);
  delay(30);
  valve.run(RELEASE);
}

void ledRED() {
  analogWrite(Rled, 130);
  analogWrite(Gled, 0);
  analogWrite(Bled, 0);
}
void ledGREEN() {
  analogWrite(Rled, 0);
  analogWrite(Gled, 130);
  analogWrite(Bled, 0);
}
void ledBLUE() {
  analogWrite(Rled, 0);
  analogWrite(Gled, 0);
  analogWrite(Bled, 130);
}
void ledOFF() {
  analogWrite(Rled, 0);
  analogWrite(Gled, 0);
  analogWrite(Bled, 0);
}
