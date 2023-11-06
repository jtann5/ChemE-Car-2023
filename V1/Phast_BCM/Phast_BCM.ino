const char* version = "Phast v1.2.1";
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

// Sensor led setup
AF_DCMotor sensorLed(3);
#define ON BACKWARD
#define OFF RELEASE

// Link to ECU
SoftwareSerial link(Rx, Tx);
char cString[20];
byte chPos = 0;
byte ch = 0;

// Commands
struct CommandFunction {
  const char* command;
  void (*function)();
};

void (*sysReset)(void) = 0;

void setup() {
  Serial.begin(9600);

  // Initialize relay
  pinMode(relayPin, OUTPUT);
  relayOFF();

  // Initialize valve
  valve.setSpeed(255);
  valveCLOSE();

  // Initialize Sensor LED
  sensorLed.setSpeed(255);
  sensorLedOff();

  // Initialize LED
  pinMode(Rled, OUTPUT);
  pinMode(Gled, OUTPUT);
  pinMode(Bled, OUTPUT);

  // Initialize link to ECU
  pinMode(Rx, INPUT);
  pinMode(Tx, OUTPUT);
  link.begin(9600);
}

// Functions for commands
void sync() {
  link.print(version);
  relayOFF();
  valveCLOSE();
  ledOFF();
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

void sensorLedOn() {
  sensorLed.run(ON);
}

void sensorLedOff() {
  sensorLed.run(OFF);
}

// Commands and the functions they call
CommandFunction commandFunctions[] = {
  {"SYN", sync},
  {"VOP", valveOPEN},
  {"RON", relayON},
  {"VCL", valveCLOSE},
  {"ROF", relayOFF},
  {"LRD", ledRED},
  {"LGN", ledGREEN},
  {"LBL", ledBLUE},
  {"LOF", ledOFF},
  {"SLN", sensorLedOn},
  {"SLF", sensorLedOff},
  {"RST", sysReset}
};

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

  for (int i = 0; i < sizeof(commandFunctions) / sizeof(commandFunctions[0]); i++) {
    if (strcmp(cString, commandFunctions[i].command) == 0) {
      if (strcmp(cString, "SYN") != 0) link.print("ACK");
      commandFunctions[i].function();
      break;
    }
  }
}