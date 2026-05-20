volatile int pulseCount = 0;

void pulseDetected() {
    pulseCount++;
}

void setup() {
    Serial.begin(9600);
    pinMode(2, INPUT);
    attachInterrupt(digitalPinToInterrupt(2),
                    pulseDetected,
                    RISING);
}

void loop() {
    // Serial.println("test");
    pulseCount = 0;

    delay(120); // count for 0.5s

    int result = pulseCount;
    Serial.println(result);

    // if(result > 200) {
    //     // high radioactivity
    // } else {
    //     // low radioactivity
    // }
}

