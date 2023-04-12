String version = "Phast v1.1.1";
String company = "TanrTech";

#include <AFMotor.h>
#include <SoftwareSerial.h>

// Pin setup
int relayPin = A0;
int Rled = A1;
int Gled = A2;
int Bled = A3;
int Rx = A4;
int Tx = A5;

AF_DCMotor valve(3);

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
    link.print(version.c_str());
    relayOFF();
    valveCLOSE();
    ledOFF();
  } else if (strcmp(cString, "Vopen") == 0) {
    link.print("ACK");
    valveOPEN();
  } else if (strcmp(cString, "Ron") == 0) {
    link.print("ACK");
    relayON();
  } else if (strcmp(cString, "Vclose") == 0) {
    link.print("ACK");
    valveCLOSE();
  } else if (strcmp(cString, "Roff") == 0) {
    link.print("ACK");
    relayOFF();
  } else if (strcmp(cString, "Lred") == 0) {
    link.print("ACK");
    ledRED();
  } else if (strcmp(cString, "Lgreen") == 0) {
    link.print("ACK");
    ledGREEN();
  } else if (strcmp(cString, "Lblue") == 0) {
    link.print("ACK");
    ledBLUE();
  } else if (strcmp(cString, "Loff") == 0) {
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
  valve.run(BACKWARD);
  delay(30);
  valve.run(RELEASE);
}

void valveOPEN() {
  valve.run(FORWARD);
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
