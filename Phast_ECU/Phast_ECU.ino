double time;

String const version = "Phast v1.1.3";
String const company = "TanrTech";

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// Reaction settings
int const reactionBaseline = 20;
int const stopValue = 5;

// Pin setup
int const xPin = A0;
int const yPin = A1;
int const buttonPin = A2;
int const photoPin = A3;
// A4 and A5 reserved for I2C LCD
int const Rx = 12;
int const Tx = 13;

// Display settings, variables, and characters
LiquidCrystal_I2C lcd(0x27,16,2);
byte copyrightChar[8] = {B11111,B10001,B10101,B10111,B10101,B10001,B11111,B00000};
byte blockChar[8] = {B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111};

// User input settings and variables
enum joystick {NONE, PRESS, LEFT, RIGHT, UP, DOWN, UNRELEASED};
bool released = true;
int const dist = 100;

// UI settings and variables
enum ui {MENU=-1, REACTION, RELAY, VALVE, RESET, INFO};
int currUI = MENU;
bool arrowPos = true; // True: arrow on top False: arrow on bottom
int selection = REACTION;
int const minS = REACTION;
int const maxS = INFO;
String const options[] = {"Reaction Center", "Relay Control", "Valve Control", "Reset", "Info"};

// Sensor settings and variables
int sensorValue = 0;

// Reaction state booleans
bool reacting = false;
bool manualStop = false;
bool reactionComplete = false;
bool baselineReached = false;

// for delays
unsigned long prev = 0UL;
unsigned long prev2 = 0UL;

// State of relay and valve
bool relayOn = false;
bool valveOpen = false;

// Link to BCM
SoftwareSerial link(Rx, Tx);
char cString[20];
byte chPos = 0;
byte ch = 0;

void(* sysReset) (void) = 0;

void setup() {
  Serial.begin(9600);   

  // Initialize joystick
  pinMode(buttonPin, INPUT);
  digitalWrite(buttonPin, HIGH);
  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);

  // Initialize sensor
  pinMode(photoPin, INPUT);

  // Initialize link to BCM
  pinMode(Rx, INPUT);
  pinMode(Tx, OUTPUT);
  link.begin(9600);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.createChar(1, copyrightChar);
  lcd.createChar(2, blockChar);

  // Startup actions
  lcd.clear();
  lcd.print("Starting up...");
  for (int i = 0; i < 3; i++) {
    ledBLUE();
    delay(250);
    ledOFF();
    delay(100);
  }

  if (strcmp(sendBCM("SYN").c_str(), version.c_str()) != 0) error("BCM ERROR", "Improper Config.");
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(version);
  lcd.setCursor(0,1);
  lcd.write(1);
  lcd.print(company);
  delay(2000);
  lcd.clear();

  // lcd.print("Hello! We're");
  // lcd.setCursor(0,1);
  // lcd.print("glad you're here");
  // delay(3000);
  // lcd.clear();
  // lcd.print("We're getting");
  // lcd.setCursor(0,1);
  // lcd.print("things ready");
  // delay(3000);
  // lcd.clear();
  // lcd.print("This won't");
  // lcd.setCursor(0,1);
  // lcd.print("take long");
  // delay(6000);
  //lcd.clear();
}

void loop() {
  switch (currUI) {
    case MENU:
      menu();
      break;
    case REACTION:
      reaction();
      break;
    case RELAY:
      relay();
      break;
    case VALVE:
      valve();
      break;
    case RESET:
      reset();
      break;
    case INFO:
      info();
      break;
    default:
      error("UI ERROR", "Unknown element");
  }
}

// Functions for all UI sections
void menu() {
  // Update display and led
  if (arrowPos) {
    refreshDisplay(">"+options[selection], " "+options[selection+1]);
  } else {
    refreshDisplay(" "+options[selection-1], ">"+options[selection]);
  }
  ledOFF();

  // Get and act upon user input
  int in = input();
  if (in == PRESS) {
    currUI = selection;
  } else if (in == UP) {
    if (selection == minS) { // start of menu
      lcd.setCursor(0,0);
      for (int i = 0; i < 16; i++) lcd.write(2);
      delay(100);
    } else if (arrowPos) {
      selection--;
    } else {
      selection--;
      arrowPos = true;
    }
  } else if (in == DOWN) {
    if (selection == maxS) { // end of menu
      lcd.setCursor(0,1);
      for (int i = 0; i < 16; i++) lcd.write(2);
      delay(100);
    } else if (arrowPos) {
      selection++;
      arrowPos = false;
    } else {
      selection++;
    }
  }
}

void reaction() {
  sensorValue = analogRead(photoPin);
  // Update display and led
  unsigned long curr = millis();
  if (curr - prev >= 250UL) { 
    prev = curr; 
    if (reacting) {
      refreshDisplay("Status: REACTING", "sensorValue: "+String(sensorValue));
      ledBLUE();

      // added for testing
      time += 0.25;
      Serial.println((String) time + ", " + sensorValue);
    } else if (manualStop) {
      refreshDisplay("Status: STOPPED", "sensorValue: "+String(sensorValue));
      ledRED();
    } else if (reactionComplete) {
      refreshDisplay("Status: COMPLETE", "sensorValue: "+String(sensorValue));
      ledGREEN();
    } else if (relayOn) {
      refreshDisplay("Status:NOT READY", "sensorValue: "+String(sensorValue));
      if (curr - prev2 >= 750UL) {
        ledRED();
        prev2 = curr;
      } else if (curr - prev2 >= 375UL) {
        ledOFF();
      }
    } else {
      refreshDisplay("Status: READY", "sensorValue: "+String(sensorValue));
      if (curr - prev2 >= 750UL) {
        ledGREEN();
        prev2 = curr;
      } else if (curr - prev2 >= 375UL) {
        ledOFF();
      }
    }
  }

  // Check status of reaction and act accordingly
  if (reacting) {
    if (baselineReached) {
      if (sensorValue <= stopValue) {
        relayOFF();
        valveCLOSE();
        reacting = false;
        reactionComplete = true;
        baselineReached = false;
      }
    } else {
      if (sensorValue >= reactionBaseline) baselineReached = true;
    }
  }

  // Get and act upon user input
  int in = input();
  if (in == PRESS) {
    if (reacting) {
      reacting = false;
      manualStop = true;
      baselineReached = false;
      relayOFF();
      valveCLOSE();
    } else if (relayOn) {
      relayOFF();
      valveCLOSE();
    } else {
      reacting = true;
      manualStop = false;
      relayON();
      valveOPEN();
      
      // added for testing
      time = 0.0;
      prev = millis();
      Serial.println("Time, SensorValue");
    }
  } else if (in == LEFT && !reacting) {
    manualStop = false;
    reactionComplete = false;
    currUI = MENU;
  }
}

void relay() {
  // Update display and led
  if (relayOn) {
    refreshDisplay("Relay Controller", "Relay: ON");
    ledGREEN();
  } else {
    refreshDisplay("Relay Controller", "Relay: OFF");
    ledRED();
  }

  // Get and act upon user input
  int in = input();
  if (in == PRESS) {
    if (relayOn) {
      relayOFF();
    } else {
      relayON();
    }
  } else if (in == LEFT) {
    currUI = MENU;
  }
}

void valve() {
  // Update display and led
  if (valveOpen) {
    refreshDisplay("Valve Controller", "Valve: OPEN");
    ledGREEN();
  } else {
    refreshDisplay("Valve Controller", "Valve: CLOSED");
    ledRED();
  }

  // Get and act upon user input
  int in = input();
  if (in == PRESS) {
    if (valveOpen) {
      valveCLOSE();
    } else {
      valveOPEN();
    }
  } else if (in == LEFT) {
    currUI = MENU;
  }
}

void reset() {
  // Update display and led
  refreshDisplay("Reset system?", "Hold to continue");
  ledRED();

  // Get and act upon user input
  int in = input();
  if (in == PRESS) {
    lcd.setCursor(0,1);
    for (int i = 0; i < 16; i++) {
      in = input();
      if (in == !UNRELEASED) break;
      lcd.write(2);
      delay(75);
      if (i%4 == 0) {
        ledOFF();
      } else {
        ledRED();
      }
      if (i == 15) {
        sendBCM("RST");
        ledOFF();
        lcd.clear();
        delay(500);
        sysReset();
      }
    }
  } else if (in == LEFT) {
    currUI = MENU;
  }
}

void info() {
  // Update display and led
  refreshDisplay(version, company);
  ledOFF();

  // Get and act upon user input
  int in = input();
  if (in == LEFT) {
    currUI = MENU;
  }
}

void error(String line1, String line2) {
  while (true) {
    refreshDisplay(line1, line2);
    int in = input();
    if (in == PRESS) sysReset();
  }
}

// Gets input from analog joystick
// Returns NONE, PRESS, LEFT, RIGHT, UP, DOWN, or UNRELEASED
int input() {
  int buttonValue = digitalRead(buttonPin);
  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);
  if (buttonValue == LOW && released) {
    released = false;
    return PRESS;
  } else if (xValue < 511-dist && released) {
    released = false;
    return LEFT;
  } else if (xValue > 511+dist && released) {
    released = false;
    return RIGHT;
  } else if (yValue < 511-dist && released) {
    released = false;
    return UP;
  } else if (yValue > 511+dist && released) {
    released = false;
    return DOWN;
  } else if (buttonValue == LOW || xValue < 511-dist || xValue > 511+dist || yValue < 511-dist || yValue > 511+dist) {
    released = false;
    return UNRELEASED;
  } else {
    released = true;
    return NONE;
  }
}

// Functions for interaction with hardware
void refreshDisplay(String in1, String in2) {
  lcd.setCursor(0,0);
  if (strlen(in1.c_str()) > 16) {
    lcd.print(in1);
  } else {
    char first[16];
    memset(first, ' ', 16);
    strncpy(first, in1.c_str(), strlen(in1.c_str()));
    lcd.print(first);
  }
  lcd.setCursor(0,1);
  if (strlen(in2.c_str()) > 16) {
    lcd.print(in2);
  } else {
    char second[16];
    memset(second, ' ', 16);
    strncpy(second, in2.c_str(), strlen(in2.c_str()));
    lcd.print(second);
  }
}

void relayON() {
  if (strcmp(sendBCM("Ron").c_str(), "ACK") == 0) relayOn = true;
}

void relayOFF() {
  if (strcmp(sendBCM("Roff").c_str(), "ACK") == 0) relayOn = false;
}

void valveOPEN() {
  if (strcmp(sendBCM("Vopen").c_str(), "ACK") == 0) valveOpen = true;
}

void valveCLOSE() {
  if (strcmp(sendBCM("Vclose").c_str(), "ACK") == 0) valveOpen = false;
}

void ledRED() {
  sendBCM("Lred");
}
void ledGREEN() {
  sendBCM("Lgreen");
}
void ledBLUE() {
  sendBCM("Lblue");
}
void ledOFF() {
  sendBCM("Loff");
}

String sendBCM(String send) {
  prev = millis();
  while (true) {
    unsigned long curr = millis();
    // Sends message every 250ms and times out at 1000ms
    if (curr - prev >= 1000UL) {
      error("BCM ERROR", "No response");
    } else if ((curr-prev)%250UL == 0){
      link.print(send);
    }
    while(link.available() > 0) {
      ch = link.read();
      cString[chPos] = ch;
      chPos++;
      delay(1);
    }
    cString[chPos] = 0; //terminate cString
    chPos = 0;
    if (strcmp(cString, "") != 0) return cString; // returns if string is not empty
  }
}
