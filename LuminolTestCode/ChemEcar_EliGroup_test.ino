unsigned int count;
double count_f;
int max_value;
double time_limit;

void setup() {
  Serial.begin(9600);
  delay(2000);
  count = 1;
  //time limit in seconds
  time_limit = 90.1;
  count_f = 0;
  max_value = 0;
  char t[60];
  sprintf(t, "T(s), Light, Max Light");
  Serial.println(t);
}

void loop() {
  int analogValue = analogRead(A0);
  if(analogValue > max_value){
    max_value = analogValue;
  }
  if((count_f <= time_limit)){
    char buffer[60];
    sprintf(buffer ,"%i, %i", analogValue, max_value);
    Serial.print(count_f);
    Serial.print(", ");
    Serial.println(buffer);
    count_f = count_f + 0.1;
    count = count + 1;
    delay(100);
  }
}
