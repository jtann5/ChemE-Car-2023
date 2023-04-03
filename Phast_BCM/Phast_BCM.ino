String version = "Phast v1.0.1";
String company = "ItWorks LLC";

#include <AFMotor.h>
AF_DCMotor valve(1);

int relayPin = A5;
int relayCon = A0;

void setup() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  valve.setSpeed(255);
  valve.run(RELEASE);
}

void loop() {
  valve.run(FORWARD);
  //delay(1000);
  if (analogRead(relayCon) > 200) {
    digitalWrite(relayPin, HIGH);
  } else {
    digitalWrite(relayPin, LOW);
  }
}
