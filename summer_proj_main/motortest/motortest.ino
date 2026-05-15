#define LEFT_DIR 5
#define LEFT_PWM 6
#define RIGHT_DIR 3
#define RIGHT_PWM 4

int speed = 255;


void reverse() {
  digitalWrite(LEFT_DIR, LOW);
  analogWrite(LEFT_PWM, speed);
  digitalWrite(RIGHT_DIR, LOW);
  analogWrite(RIGHT_PWM, speed);
  // server.send(200, F("text/plain"), F("REVERSE"));
}


void forward() {
  digitalWrite(LEFT_DIR, HIGH);
  analogWrite(LEFT_PWM, speed);
  digitalWrite(RIGHT_DIR, HIGH);
  analogWrite(RIGHT_PWM, speed);
  // server.send(200, F("text/plain"), F("FORWARD"));
}


void right() {
  digitalWrite(LEFT_DIR, LOW);
  analogWrite(LEFT_PWM, speed);
  digitalWrite(RIGHT_DIR, HIGH);
  analogWrite(RIGHT_PWM, speed);
  // server.send(200, F("text/plain"), F("RIGHT"));
}


void left() {
  digitalWrite(LEFT_DIR, HIGH);
  analogWrite(LEFT_PWM, speed);
  digitalWrite(RIGHT_DIR, LOW);
  analogWrite(RIGHT_PWM, speed);
  // server.send(200, F("text/plain"), F("LEFT"));
}

void stop() {
  analogWrite(LEFT_PWM, 0);
  analogWrite(RIGHT_PWM, 0);
  // server.send(200, F("text/plain"), F("STOP"));
}

void setup() {
  // put your setup code here, to run once:
  pinMode(LEFT_DIR, OUTPUT);
  pinMode(LEFT_PWM, OUTPUT);
  pinMode(RIGHT_DIR, OUTPUT);
  pinMode(RIGHT_PWM, OUTPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
  forward();
  delay(2000);
  left();
  delay(2000);
  right();
  delay(2000);
  reverse();
  delay(2000);
  stop();
  delay(2000);


}
