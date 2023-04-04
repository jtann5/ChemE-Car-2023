String version = "Phast v1.0.2";
String company = "ItWorks LLC";

#include<Wire.h>
#include <LiquidCrystal.h>

// Reaction settings
int reactionBaseline = 500;
int stopValue = 100;

// Pin setup
int xPin = A0;
int yPin = A1;
int buttonPin = A2;
const int rs = 7, e = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
int Rled = 9;
int Gled = 10;
int Bled = 11;
int photoPin = A3;
int relayPin = A4;
int valvePin = A5;

// Display settings, variables, and characters
LiquidCrystal lcd(rs, e, d4, d5, d6, d7);
bool updateDisplay = true;
byte copyrightChar[8] = {B11111,B10001,B10101,B10111,B10101,B10001,B11111,B00000};
byte blockChar[8] = {B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111};

// User input settings and variables
#define NONE 0
#define PRESS 1
#define LEFT 2
#define RIGHT 3
#define UP 4
#define DOWN 5
#define UNRELEASED 6
bool released = true;
int dist = 250;

// UI settings and variables
#define MENU -1
#define REACTION 0
#define RELAY 1
#define VALVE 2
#define RESET 3
#define INFO 4
int currUI = MENU;
bool arrowPos = true; // True: arrow on top False: arrow on bottom
int selection = REACTION;
int minS = REACTION;
int maxS = INFO;
String options[] = {"Reaction Center", "Relay Control", "Valve Control", "Reset", "Info"};

// Sensor settings and variables
int sensorValue = 0;

// State booleans
bool reacting = false;
bool manualStop = false;
bool reactionComplete = false;
bool baselineReached = false;
unsigned long prev = 0UL;
unsigned long prev2 = 0UL;

void(* sysReset) (void) = 0;

void setup() {
  // Joystick setup
  pinMode(buttonPin, INPUT);
  digitalWrite(buttonPin, HIGH);
  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);

  pinMode(photoPin, INPUT);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  pinMode(valvePin, OUTPUT);
  digitalWrite(valvePin, LOW);

  lcd.begin(16, 2);
  lcd.createChar(1, copyrightChar);
  lcd.createChar(2, blockChar);

  // RGB led setup
  pinMode(Rled, OUTPUT);
  pinMode(Gled, OUTPUT);
  pinMode(Bled, OUTPUT);

  Serial.begin(9600);

  lcd.clear();
  lcd.print("Starting up...");
  for (int i = 0; i < 3; i++) {
    ledBLUE();
    delay(250);
    ledOFF();
    delay(100);
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(version);
  lcd.setCursor(0,1);
  lcd.write(1);
  lcd.print(company);
  delay(2000);

  // lcd.clear();
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
  lcd.clear();
  ledOFF();
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
      refreshDisplay("Critical Error", "Restarting System");
      delay(1000);
      sysReset();
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
    sensorValue = analogRead(photoPin);
    if (reacting) {
      refreshDisplay("Status: REACTING", "sensorValue: "+String(sensorValue));
      ledBLUE();
    } else if (manualStop) {
      refreshDisplay("Status: STOPPED", "sensorValue: "+String(sensorValue));
      ledRED();
    } else if (reactionComplete) {
      refreshDisplay("Status: COMPLETE", "sensorValue: "+String(sensorValue));
      ledGREEN();
    } else if (digitalRead(relayPin) == HIGH) {
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
  // Status: STOPPED, READY, REACTING, NOT READY, COMPLETED
  // sensorValue: XXXX

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
    } else if (digitalRead(relayPin) == HIGH) {
      relayOFF();
      valveCLOSE();
    } else {
      reacting = true;
      manualStop = false;
      relayON();
      valveOPEN();
    }
  } else if (in == LEFT && !reacting) {
    manualStop = false;
    reactionComplete = false;
    currUI = MENU;
  }
}

void relay() {
  // Update display and led
  if (digitalRead(relayPin) == HIGH) {
    refreshDisplay("Relay Controller", "Relay: ON");
    ledGREEN();
  } else {
    refreshDisplay("Relay Controller", "Relay: OFF");
    ledRED();
  }

  // Get and act upon user input
  int in = input();
  if (in == PRESS) {
    if (digitalRead(relayPin) == HIGH) {
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
  if (digitalRead(relayPin) == HIGH) {
    refreshDisplay("Valve Controller", "Valve: OPEN");
    ledGREEN();
  } else {
    refreshDisplay("Valve Controller", "Valve: CLOSED");
    ledRED();
  }

  // Get and act upon user input
  int in = input();
  if (in == PRESS) {
    if (digitalRead(relayPin) == HIGH) {
      relayOFF();
    } else {
      relayON();
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
    analogWrite(Gled, 0);
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
  digitalWrite(relayPin, HIGH);
}

void relayOFF() {
  digitalWrite(relayPin, LOW);
}

void valveOPEN() {
  digitalWrite(valvePin, HIGH);
}

void valveCLOSE() {
  digitalWrite(valvePin, LOW);
}

void ledRED() {
  analogWrite(Rled, 200);
  analogWrite(Gled, 0);
  analogWrite(Bled, 0);
}
void ledGREEN() {
  analogWrite(Rled, 0);
  analogWrite(Gled, 50);
  analogWrite(Bled, 0);
}
void ledBLUE() {
  analogWrite(Rled, 0);
  analogWrite(Gled, 0);
  analogWrite(Bled, 50);
}
void ledOFF() {
  analogWrite(Rled, 0);
  analogWrite(Gled, 0);
  analogWrite(Bled, 0);
}
