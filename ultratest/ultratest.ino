int lastultratime = 0;
bool near = false;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:

  int ultrareading = analogRead(A0);
  if (ultrareading > 15) {
    near = true;
    lastultratime = millis();
  }
  if (ultrareading < 10 && millis() - lastultratime > 100) {
    near = false;
  }
  Serial.print(ultrareading);
  Serial.print(" ");
  Serial.println(near);



} 
