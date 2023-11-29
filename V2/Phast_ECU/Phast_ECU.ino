const char* version = "Phast v2.0.0";
const char* company = "TanrTech";

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// Reaction settings
int const reactionBaseline = 50;
int const stopValue = 20;

// Pin setup
int const xPin = A0;
int const yPin = A1;
int const buttonPin = A2;
int const photoPin = A3;
// A4 and A5 reserved for I2C LCD
int const Rx = 12;
int const Tx = 13;

// Display settings, variables, and custom characters
LiquidCrystal_I2C lcd(0x27,16,2);
byte copyrightChar[8] = {B11111,B10001,B10101,B10111,B10101,B10001,B11111,B00000};
byte blockChar[8] = {B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111};

// User input settings and variables
enum joystick {NONE, PRESS, LEFT, RIGHT, UP, DOWN, UNRELEASED};
bool released = true;
int const dist = 75;
int const input_delay = 50;
int input_timer = 0;


// UI settings and variables
enum ui {MENU=-1, REACTION, RELAY, VALVE, RESET, INFO};
const char* options[] = {"Reaction Center", "Relay Control", "Valve Control", "Reset", "Info"};
int currUI = MENU;
bool arrowPos = true; // True: arrow on top False: arrow on bottom
int selection = REACTION;
int const minS = REACTION;
int const maxS = INFO;

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

  // Initialize LCD and special characters
  lcd.init();
  lcd.backlight();
  lcd.createChar(1, copyrightChar);
  lcd.createChar(2, blockChar);

  // Startup UI Screen
  lcd.clear();
  lcd.print("Starting up...");
  for (int i = 0; i < 3; i++) {
    ledBLUE();
    delay(250);
    ledOFF();
    delay(100);
  }
  if (strcmp(sendBCM("SYN").c_str(), version) != 0) error("BCM ERROR", "Improper Config.");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(version);
  lcd.setCursor(0,1);
  lcd.write(1);
  lcd.print(company);
  delay(2000);
  lcd.clear();
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
      currUI = MENU;
  }
}

// Functions for all UI sections
void menu() {
  // Update display and led
    auto updateDisplay = [&](){
      if (arrowPos) {
        refreshDisplay(">"+(String)options[selection], " "+(String)options[selection+1]);
      } else {
        refreshDisplay(" "+(String)options[selection-1], ">"+(String)options[selection]);
      }
    };
    updateDisplay();
    ledOFF();

  while (true) {
    // Get and act upon user input
    int in = input();
    if (in == PRESS) {
      currUI = selection;
      break;
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
      updateDisplay();
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
      updateDisplay();
    }
  }
}

void reaction() {
  // Reaction state booleans
  bool reacting = false;
  bool manualStop = false;
  bool reactionComplete = false;
  bool baselineReached = false;

  // Sensor settings and variables
  int sensorValue = 0;
  int sensorValues[10] = {0};
  int index = 0;

  // For ui and sensor reading timing
  unsigned long prev = 0UL;
  unsigned long prev2 = 0UL;
  unsigned long prev3 = 0UL;

  double time;

  while (true) {
    unsigned long curr = millis();
    if (curr - prev3 >= 25UL) {
      prev3 = curr;
      sensorValues[index++] = analogRead(photoPin);
      if (index >= sizeof(sensorValues)/sizeof(sensorValues[0])) index = 0;
    }
    sensorValue = median(sensorValues);
    // Update display and led
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
        // if (sensorValue <= stopValue) {
        //   relayOFF();
        //   valveCLOSE();
        //   reacting = false;
        //   reactionComplete = true;
        //   baselineReached = false;
        // }
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
        sensorLedOff();
      } else if (relayOn) {
        relayOFF();
        valveCLOSE();
        sensorLedOff();
      } else {
        reacting = true;
        manualStop = false;
        relayON();
        valveOPEN();
        sensorLedOn();
        
        // added for testing
        time = 0.0;
        prev = millis();
        Serial.println("Time, SensorValue");
      }
    } else if (in == LEFT && !reacting) {
      manualStop = false;
      reactionComplete = false;
      currUI = MENU;
      break;
    }
  }
}

void swap(int* xp, int* yp) {
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

// Function to perform Selection Sort
void selectionSort(int arr[], int n) {
    int i, j, min_idx;
    // One by one move boundary of unsorted subarray
    for (i = 0; i < n - 1; i++) {
        // Find the minimum element in unsorted array
        min_idx = i;
        for (j = i + 1; j < n; j++)
            if (arr[j] < arr[min_idx])
                min_idx = j;
        // Swap the found minimum element with the first element
        swap(&arr[min_idx], &arr[i]);
    }
}

int median(int in[]) {
  // Get size of array and copy it
  int size = sizeof(in)/sizeof(in[0]);
  int arr[size];
  for (int i = 0; i < size; i++) arr[i] = in[i];  

  selectionSort(arr, size);
   if (size % 2 != 0) return arr[size/2];
   return arr[(size-1)/2];
}



void relay() {
  // Update display and led
  auto updateDisplay = [&](){
    if (relayOn) {
      refreshDisplay("Relay Controller", "Relay: ON");
      ledGREEN();
    } else {
      refreshDisplay("Relay Controller", "Relay: OFF");
      ledRED();
    }
  };
  updateDisplay();

  while (true) {
    // Get and act upon user input
    int in = input();
    if (in == PRESS) {
      if (relayOn) {
        relayOFF();
      } else {
        relayON();
      }
      updateDisplay();
    } else if (in == LEFT) {
      currUI = MENU;
      break;
    }
  }
}

void valve() {
  // Update display and led
  auto updateDisplay = [&](){
    if (valveOpen) {
      refreshDisplay("Valve Controller", "Valve: OPEN");
      ledGREEN();
    } else {
      refreshDisplay("Valve Controller", "Valve: CLOSED");
      ledRED();
    }
  };
  updateDisplay();

  while (true) {
    // Get and act upon user input
    int in = input();
    if (in == PRESS) {
      if (valveOpen) {
        valveCLOSE();
      } else {
        valveOPEN();
      }
      updateDisplay();
    } else if (in == LEFT) {
      currUI = MENU;
      break;
    }
  }
}

void reset() {
  // Update display and led
  auto updateDisplay = [&](){
    refreshDisplay("Reset system?", "Hold to continue");
    ledRED();
  };
  updateDisplay();

  while (true) {
    // Get and act upon user input
    int in = input();
    if (in == PRESS) {
      lcd.setCursor(0,1);
      for (int i = 0; i < 16; i++) {
        in = input();
        if (in == !UNRELEASED) {
          updateDisplay();
          break;
        }
        lcd.write(2);
        delay(50);
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
      break;
    }
  }
}

void info() {
  // Update display and led
  auto updateDisplay = [&](){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(version);
    lcd.setCursor(0,1);
    lcd.write(1);
    lcd.print(company);
    ledOFF();
  };
  updateDisplay();

  while (true) {
    // Get and act upon user input
    int in = input();
    if (in == PRESS) {
      for (int i = 0; i < 16; i++) {
        in = input();
        if (in == !UNRELEASED) break;
        delay(50);
        if (i == 15) {
          TakeOnMe();
          updateDisplay();
        }
      }
    } else if (in == LEFT) {
      currUI = MENU;
      break;
    }
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
    input_timer = millis();
    return PRESS;
  } else if (xValue < 511-dist && released) {
    released = false;
    input_timer = millis();
    return LEFT;
  } else if (xValue > 511+dist && released) {
    released = false;
    input_timer = millis();
    return RIGHT;
  } else if (yValue < 511-dist && released) {
    released = false;
    input_timer = millis();
    return UP;
  } else if (yValue > 511+dist && released) {
    released = false;
    input_timer = millis();
    return DOWN;
  } else if (buttonValue == LOW || xValue < 511-dist || xValue > 511+dist || yValue < 511-dist || yValue > 511+dist) {
    released = false;
    input_timer = millis();
    return UNRELEASED;
  } else if (millis() - input_timer <= input_delay) {
    return NONE;
  } else {
    released = true;
    return NONE;
  }
}

// ******************** Functions for interaction with hardware below ********************
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

String sendBCM(String send) {
  unsigned long prev = millis();
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

void relayON() {
  if (strcmp(sendBCM("RON").c_str(), "ACK") == 0) relayOn = true;
}

void relayOFF() {
  if (strcmp(sendBCM("ROF").c_str(), "ACK") == 0) relayOn = false;
}

void valveOPEN() {
  if (strcmp(sendBCM("VOP").c_str(), "ACK") == 0) valveOpen = true;
}

void valveCLOSE() {
  if (strcmp(sendBCM("VCL").c_str(), "ACK") == 0) valveOpen = false;
}

void ledRED() {
  sendBCM("LRD");
}

void ledGREEN() {
  sendBCM("LGN");
}
void ledBLUE() {
  sendBCM("LBL");
}
void ledOFF() {
  sendBCM("LOF");
}

void sensorLedOn() {
  sendBCM("SLN");
}

void sensorLedOff() {
 sendBCM("SLF");
}


// ******************** Code below for Take on Me by A-ha ********************
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST      0

void TakeOnMe() {
  int count = 0;

  // change this to make the song slower or faster
  int tempo = 169;

  // change this to whichever pin you want to use
  int buzzer = 10;

  // notes of the moledy followed by the duration.
  // a 4 means a quarter note, 8 an eighteenth , 16 sixteenth, so on
  // !!negative numbers are used to represent dotted notes,
  // so -4 means a dotted quarter note, that is, a quarter plus an eighteenth!!
  int melody[] = {

    // Take on me, by A-ha
    // Score available at https://musescore.com/user/27103612/scores/4834399
    // Arranged by Edward Truong

    NOTE_FS5,8, NOTE_FS5,8,NOTE_D5,8, NOTE_B4,8, REST,8, NOTE_B4,8, REST,8, NOTE_E5,8, 
    REST,8, NOTE_E5,8, REST,8, NOTE_E5,8, NOTE_GS5,8, NOTE_GS5,8, NOTE_A5,8, NOTE_B5,8,
    NOTE_A5,8, NOTE_A5,8, NOTE_A5,8, NOTE_E5,8, REST,8, NOTE_D5,8, REST,8, NOTE_FS5,8, 
    REST,8, NOTE_FS5,8, REST,8, NOTE_FS5,8, NOTE_E5,8, NOTE_E5,8, NOTE_FS5,8, NOTE_E5,8,
    NOTE_FS5,8, NOTE_FS5,8,NOTE_D5,8, NOTE_B4,8, REST,8, NOTE_B4,8, REST,8, NOTE_E5,8, 
    
    REST,8, NOTE_E5,8, REST,8, NOTE_E5,8, NOTE_GS5,8, NOTE_GS5,8, NOTE_A5,8, NOTE_B5,8,
    NOTE_A5,8, NOTE_A5,8, NOTE_A5,8, NOTE_E5,8, REST,8, NOTE_D5,8, REST,8, NOTE_FS5,8, 
    REST,8, NOTE_FS5,8, REST,8, NOTE_FS5,8, NOTE_E5,8, NOTE_E5,8, NOTE_FS5,8, NOTE_E5,8,
    NOTE_FS5,8, NOTE_FS5,8,NOTE_D5,8, NOTE_B4,8, REST,8, NOTE_B4,8, REST,8, NOTE_E5,8, 
    REST,8, NOTE_E5,8, REST,8, NOTE_E5,8, NOTE_GS5,8, NOTE_GS5,8, NOTE_A5,8, NOTE_B5,8,
    
    NOTE_A5,8, NOTE_A5,8, NOTE_A5,8, NOTE_E5,8, REST,8, NOTE_D5,8, REST,8, NOTE_FS5,8, 
    REST,8, NOTE_FS5,8, REST,8, NOTE_FS5,8, NOTE_E5,8, NOTE_E5,8, NOTE_FS5,8, NOTE_E5,8,
    
  };

  // sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
  // there are two values per note (pitch and duration), so for each note there are four bytes
  int notes = sizeof(melody) / sizeof(melody[0]) / 2;

  // this calculates the duration of a whole note in ms
  int wholenote = (60000 * 4) / tempo;

  int divider = 0, noteDuration = 0;

  // iterate over the notes of the melody.
  // Remember, the array is twice the number of notes (notes + durations)
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {

    // calculates the duration of each note
    divider = melody[thisNote + 1];
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    // we only play the note for 90% of the duration, leaving 10% as a pause
    tone(buzzer, melody[thisNote], noteDuration * 0.9);

    // Wait for the specief duration before playing the next note.
    delay(noteDuration);

    // stop the waveform generation before the next note.
    noTone(buzzer);

    if (count == 0) {
      link.print("LRD");
      count++;
    } else if (count == 1) {
      link.print("LGN");
      count++;
    } else if (count == 2) {
      link.print("LBL");
      count = 0;
    }

    // stop playback if joystick is pushed down
    if (input() == DOWN) break;
  }
  ledOFF();
}