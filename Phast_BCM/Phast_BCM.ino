String version = "Phast v1.0.2";
String company = "ItWorks LLC";

#include <AFMotor.h>
AF_DCMotor valve(3);

int relayPin = A5;
int relayCon = A0;
int d = 150;

void setup() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  valve.setSpeed(255);
  valve.run(RELEASE);
}

void loop() {
  // int on = 5;
  // d-=1;
  // if (d < 25) d = 25;
  // valve.run(FORWARD);
  // //forward is open to freaky shit
  // //reverse is closed af
  // delay(on);
  // valve.run(RELEASE);
  // delay(d);
  // valve.run(BACKWARD);
  // delay(on);
  // valve.run(RELEASE);
  // delay(d);
  if (analogRead(relayCon) > 200) {
    digitalWrite(relayPin, HIGH);
  } else {
    digitalWrite(relayPin, LOW);
  }
}
