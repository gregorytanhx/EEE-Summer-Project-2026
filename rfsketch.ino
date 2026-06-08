// ============================================================
// Rock radio receiver — EEELunarRover
// 89 kHz ASK → demodulator → comparator (inverted) → Metro pin 0
//
// Decodes 4-character frames in the format #XYZ
// Age in billions of years = X.YZ
//
// Wiring:
//   Comparator output (0/3.3 V, inverted polarity) → Metro M0 digital pin 0
//   Analog GND → Metro GND (shared reference)
//
// Uses manual bit-bang UART (digitalRead + delayMicroseconds) because:
//   - Metro M0's hardware UART (Serial1) can't handle inverted polarity
//   - SoftwareSerial library not available for SAMD21
// ============================================================

const int RX_PIN = 0;
const long BAUD = 600;
const long BIT_PERIOD_US = 1000000L / BAUD;   // 1667 µs per bit at 600 baud
const long HALF_BIT_US   = BIT_PERIOD_US / 2; // 833 µs

// Line idles LOW on wire (inverted from standard UART)
// Set to false if your hardware idles HIGH instead
const bool INVERTED = true;

char buf[5];                       // 4 chars + null terminator
int idx = 0;
bool framed = false;
unsigned long lastByteTime = 0;

// Returns the LOGICAL state of the line: TRUE = idle, FALSE = active/start
inline bool lineIsIdle() {
  bool raw = digitalRead(RX_PIN);
  if (INVERTED) raw = !raw;
  return raw;
}

// Wait for a start bit, then sample 8 data bits. Returns the byte, or -1 on timeout.
int readByte(unsigned long timeout_ms) {
  unsigned long deadline = millis() + timeout_ms;
  
  // Ensure line is currently idle (don't jump into the middle of a byte)
  while (!lineIsIdle()) {
    if (millis() > deadline) return -1;
  }
  
  // Wait for line to go active (start bit)
  while (lineIsIdle()) {
    if (millis() > deadline) return -1;
  }
  
  // Land in middle of start bit and verify still active
  delayMicroseconds(HALF_BIT_US);
  if (lineIsIdle()) return -1;
  
  // Sample 8 data bits, LSB first
  byte b = 0;
  for (int i = 0; i < 8; i++) {
    delayMicroseconds(BIT_PERIOD_US);
    if (lineIsIdle()) b |= (1 << i);   // logical 1 = idle level
  }
  
  // Wait through stop bit (not verified — start bit of next frame re-syncs)
  delayMicroseconds(BIT_PERIOD_US);
  
  return b;
}

void setup() {
  Serial.begin(115200);              // USB debug to laptop
  while (!Serial) { ; }              // wait for USB enumeration on Metro M0
  
  pinMode(RX_PIN, INPUT);
  
  Serial.println("==============================================");
  Serial.print("  Rock radio receiver — ");
  Serial.print(BAUD);
  Serial.print(" baud, ");
  Serial.print(INVERTED ? "INVERTED" : "normal");
  Serial.println(" polarity");
  Serial.println("  Waiting for transmission (#XYZ format)...");
  Serial.println("==============================================");
}

void loop() {
  int b = readByte(2000);
  
  if (b < 0) {
    Serial.print(".");
    return;
  }
  
  // Print raw byte (helpful for diagnostics; comment out if you only want age lines)
  Serial.print("[0x");
  if (b < 0x10) Serial.print('0');
  Serial.print(b, HEX);
  Serial.print(" '");
  if (b >= 32 && b < 127) Serial.print((char)b);
  else Serial.print('?');
  Serial.print("'] ");
  
  char c = (char)b;
  lastByteTime = millis();
  
  // Frame starts with '#'
  if (c == '#') {
    idx = 0;
    framed = true;
  }
  
  if (framed && idx < 4) {
    buf[idx++] = c;
    
    if (idx == 4) {
      buf[4] = '\0';
      
      // Validate: '#' followed by three digits
      if (buf[0] == '#' && isDigit(buf[1]) && isDigit(buf[2]) && isDigit(buf[3])) {
        Serial.println();
        Serial.print(">>> Rock age = ");
        Serial.print(buf[1]);
        Serial.print('.');
        Serial.print(buf[2]);
        Serial.print(buf[3]);
        Serial.println(" billion years");
      } else {
        Serial.print("(frame error) ");
      }
      framed = false;
    }
  }
}

