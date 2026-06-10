// #define USE_HOTSPOT
#define USE_WIFI_NINA false
#define USE_WIFI101 true
#include <WiFiWebServer.h>

#define LEFT_DIR 8
#define LEFT_PWM 9
#define RIGHT_DIR 3
#define RIGHT_PWM 4
#define IR_INPUT 2

#define ULTRA_SWITCH 7

#define ULTRA_THRES 20

#define MAG_THRES 15
#define MAG_STABLE_VAL 528

#define right_speed_modifier 0.9


#ifdef USE_HOTSPOT
const int groupNumber = 0;
const char ssid[] = "hotspot";
const char pass[] = "freew1f1";

#else

const int groupNumber = 24;  // Set your group number to make the IP address constant - only do this on the EEERover network
const char ssid[] = "EEERover";
const char pass[] = "exhibition";
#endif

// ─── Movement ───
int speed = 255;
int twoinputspeed = speed;   // inner-wheel speed for diagonal turns (set by turn ratio)

// ─── Sensor data ───
volatile int IRPulseCount = 0;   // volatile: modified inside the interrupt
const int IRSampleTime = 300;    // measurement window in ms
long lastIRTime = 0;
float IRPulseRate = 300;
bool ultraDetected = false;
int ultraReading;
const int ultraSampleTime = 500;
int ultraMin = 1023;
int ultraMax = 0;
int magReading;

const int numReadings = 30;
int readings [numReadings];
int readIdx = 0;
long total = 0;


int magDir = 0; // 1 = up, -1 = down, 0 = no magnet;
String rockAge = "0.00";
long lastUltraTime = 0;

long lastRockTime = 0;

// ─── Radio toggle ───
// When false, updateRadio() is completely skipped in loop().
// This prevents readByte(2000) from blocking the web server for 2 seconds.
bool radioEnabled = false;

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


//Webpage to return when root is requested
const char webpage[] PROGMEM = R"rawliteral(
 <!DOCTYPE html>
<html>
  <head>
    <style>
      body {
        background: #0a0a0b;
        color: #e8e8ea;
        font-family: monospace;
        margin: 0 auto;
        padding: 32px 24px;
        max-width: 720px;
      }
      h1 {
        font-size: 13px;
        letter-spacing: 0.25em;
        color: #ffb547;
        padding-bottom: 14px;
        border-bottom: 1px solid #222;
        margin: 0 0 24px;
      }
      .layout {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 32px;
      }
      .column {
        min-width: 0;
      }
      .section-header {
        font-size: 13px;
        color: #ffb547;
        letter-spacing: 0.2em;
        text-transform: uppercase;
        font-weight: 700;
        padding-bottom: 10px;
        border-bottom: 1px solid #2a2a30;
        margin-bottom: 16px;
      }
      .btn {
        background: transparent;
        border: 1px solid #333;
        color: #888;
        font-family: monospace;
        padding: 12px 24px;
        font-size: 12px;
        letter-spacing: 0.15em;
        text-transform: uppercase;
        cursor: pointer;
        margin: 0 4px 16px 0;
      }
      .btn:hover {
        border-color: #ffb547;
        color: #ffb547;
      }
      #led_state, #move_state, #speed_val, #turn_val {
        color: #ffb547;
        font-weight: 700;
      }
      .label {
        color: #888;
        font-size: 11px;
        letter-spacing: 0.15em;
        text-transform: uppercase;
        display: inline-block;
        margin-right: 8px;
      }
      .status {
        padding-top: 0;
        line-height: 2;
      }
      .pad {
        margin-top: 24px;
        padding-top: 16px;
        border-top: 1px dashed #222;
      }
      .pad-grid {
        display: grid;
        grid-template-columns: repeat(3, 48px);
        grid-template-rows: repeat(2, 48px);
        gap: 6px;
        justify-content: center;
        margin-top: 12px;
      }
      .key {
        background: transparent;
        border: 1px solid #333;
        color: #888;
        font-family: monospace;
        font-size: 14px;
        font-weight: 700;
        cursor: pointer;
        user-select: none;
        transition: background 0.08s, color 0.08s, border-color 0.08s;
        display: flex;
        align-items: center;
        justify-content: center;
      }
      .key.active {
        background: #ffb547;
        border-color: #ffb547;
        color: #0a0a0b;
      }
      .k-w { grid-column: 2; grid-row: 1; }
      .k-a { grid-column: 1; grid-row: 2; }
      .k-s { grid-column: 2; grid-row: 2; }
      .k-d { grid-column: 3; grid-row: 2; }
      .pad-hint {
        text-align: center;
        color: #555;
        font-size: 10px;
        letter-spacing: 0.2em;
        margin-top: 12px;
      }
      .speed, .turnratio {
        margin-top: 24px;
        padding-top: 16px;
        border-top: 1px dashed #222;
      }
      .speed-head, .turnratio-head {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: 10px;
      }
      input[type="range"] {
        -webkit-appearance: none;
        appearance: none;
        width: 100%;
        height: 2px;
        background: #333;
        outline: none;
        cursor: pointer;
      }
      input[type="range"]::-webkit-slider-thumb {
        -webkit-appearance: none;
        appearance: none;
        width: 14px;
        height: 14px;
        background: #ffb547;
        border-radius: 50%;
        cursor: pointer;
      }
      input[type="range"]::-moz-range-thumb {
        width: 14px;
        height: 14px;
        background: #ffb547;
        border-radius: 50%;
        cursor: pointer;
        border: none;
      }
      /* Rock classification card */
      .rock-card {
        padding: 16px;
        border: 1px solid #2a2a30;
        margin-bottom: 16px;
        transition: border-color 0.3s, background 0.3s;
      }
      .rock-label {
        font-size: 10px;
        color: #888;
        letter-spacing: 0.25em;
        text-transform: uppercase;
        margin-bottom: 6px;
      }
      .rock-name {
        font-size: 24px;
        font-weight: 700;
        letter-spacing: 0.05em;
        text-transform: uppercase;
        color: #888;
        transition: color 0.3s;
        margin-bottom: 12px;
      }
      .rock-age {
        padding-top: 10px;
        border-top: 1px dashed #2a2a30;
      }
      .rock-age-label {
        font-size: 10px;
        color: #888;
        letter-spacing: 0.2em;
        text-transform: uppercase;
        margin-bottom: 4px;
      }
      .rock-age-value {
        font-size: 18px;
        font-weight: 700;
        color: #ffb547;
      }
      .rock-age-unit {
        font-size: 11px;
        color: #888;
        font-weight: 400;
        letter-spacing: 0.1em;
        margin-left: 4px;
      }
      /* Per-rock color schemes */
      .rock-card.basaltoid { border-color: #5a7a99; background: rgba(90,122,153,0.08); }
      .rock-card.basaltoid .rock-name { color: #8eb3d1; }
      .rock-card.gravion   { border-color: #c46a3e; background: rgba(196,106,62,0.08); }
      .rock-card.gravion   .rock-name { color: #e89b6f; }
      .rock-card.regolix   { border-color: #c4a878; background: rgba(196,168,120,0.08); }
      .rock-card.regolix   .rock-name { color: #e0c89a; }
      .rock-card.lunarite  { border-color: #d8d8e0; background: rgba(216,216,224,0.08); }
      .rock-card.lunarite  .rock-name { color: #ffffff; }
      .rock-card.unknown   { border-color: #444; }
      .rock-card.unknown   .rock-name { color: #555; }
      /* Sensor rows */
      .sensor-row {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 10px 0;
        border-bottom: 1px dashed #1a1a1a;
      }
      .sensor-row:last-child {
        border-bottom: none;
      }
      .sensor-name {
        font-size: 10px;
        color: #888;
        letter-spacing: 0.2em;
        text-transform: uppercase;
        margin-bottom: 4px;
      }
      .sensor-val {
        font-size: 15px;
        color: #ffb547;
        font-weight: 700;
      }
      .sensor-unit {
        font-size: 10px;
        color: #888;
        font-weight: 400;
        margin-left: 4px;
      }
      .mag-indicator {
        font-size: 18px;
        color: #ffb547;
        font-weight: 700;
        letter-spacing: 0.1em;
      }

      /* ─── Radio toggle button ─── */
      /* OFF state: dull border, grey text */
      #radio_toggle {
        background: transparent;
        border: 1px solid #333;
        color: #888;
        font-family: monospace;
        padding: 12px 24px;
        font-size: 12px;
        letter-spacing: 0.15em;
        text-transform: uppercase;
        cursor: pointer;
        margin: 0 0 16px 0;
        width: 100%;
        text-align: left;
        display: flex;
        justify-content: space-between;
        align-items: center;
      }
      /* ON state: amber border and text to match the UI accent colour */
      #radio_toggle.on {
        border-color: #ffb547;
        color: #ffb547;
      }
      #radio_toggle .radio-dot {
        width: 8px;
        height: 8px;
        border-radius: 50%;
        background: #333;
        display: inline-block;
      }
      #radio_toggle.on .radio-dot {
        background: #ffb547;
        /* Pulsing glow so you can tell it's actively listening */
        box-shadow: 0 0 6px #ffb547;
        animation: pulse 1.2s ease-in-out infinite;
      }
      @keyframes pulse {
        0%, 100% { opacity: 1; }
        50%       { opacity: 0.3; }
      }

      @media (max-width: 600px) {
        .layout {
          grid-template-columns: 1fr;
          gap: 0;
        }
      }
    </style>
  </head>
  <body>
    <h1>LUNAR &middot; ROVER</h1>

    <div class="layout">

      <div class="column">
        <div class="section-header">Controls</div>

        <button class="btn" onclick="ledOn()">LED On</button>
        <button class="btn" onclick="ledOff()">LED Off</button>

        <div class="status">
          <span class="label">LED State:</span> <span id="led_state">OFF</span>
          <br>
          <span class="label">Movement:</span> <span id="move_state">STOP</span>
        </div>

        <div class="pad">
          <div class="pad-grid">
            <div class="key k-w" data-k="w">W</div>
            <div class="key k-a" data-k="a">A</div>
            <div class="key k-s" data-k="s">S</div>
            <div class="key k-d" data-k="d">D</div>
          </div>
          <div class="pad-hint">USE W A S D TO DRIVE</div>
        </div>

        <div class="speed">
          <div class="speed-head">
            <span class="label">Throttle</span>
            <span id="speed_val">255</span>
          </div>
          <input type="range" id="speed_slider" min="80" max="255" value="255">
        </div>

        <div class="turnratio">
          <div class="turnratio-head">
            <span class="label">Turning Ratio</span>
            <span id="turn_val">1</span>
          </div>
          <input type="range" id="turn_slider" min="0" max="1" step="0.01" value="1">
        </div>
      </div>

      <div class="column">
        <div class="section-header">Sensor Data</div>

        <!-- ─── Radio toggle ─── -->
        <!-- Clicking this button sends GET /radiotoggle to the Arduino.   -->
        <!-- The Arduino flips its radioEnabled flag and replies "1" or "0" -->
        <!-- so the button can update its own appearance to match.          -->
        <button id="radio_toggle" onclick="toggleRadio()">
          <span>Radio Receiver</span>
          <span class="radio-dot"></span>
        </button>

        <div id="rock_card" class="rock-card unknown">
          <div class="rock-label">Identified As</div>
          <div id="rock_name" class="rock-name">--</div>
          <div class="rock-age">
            <div class="rock-age-label">Age</div>
            <div>
              <span class="rock-age-value" id="age_val">--</span>
              <span class="rock-age-unit">BILLION YEARS</span>
            </div>
          </div>
        </div>

        <div class="sensor-row">
          <div class="sensor-info">
            <div class="sensor-name">Infrared (&lambda;)</div>
            <div class="sensor-val"><span id="ir_val">--</span><span class="sensor-unit">s&#8315;&sup1;</span></div>
          </div>
        </div>

        <div class="sensor-row">
          <div class="sensor-info">
            <div class="sensor-name">Ultrasound</div>
            <div class="sensor-val"><span id="us_val">--</span></div>
          </div>
        </div>

        <div class="sensor-row">
          <div class="sensor-info">
            <div class="sensor-name">Magnetic</div>
            <div class="mag-indicator" id="mag_val">--</div>
          </div>
        </div>
      </div>

    </div>

  <script>
  function send(cmd, elementId) {
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        if (elementId) document.getElementById(elementId).innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", cmd, true);
    xhttp.send();
  }

  function setKey(k, on) {
    const el = document.querySelector('.key[data-k="' + k + '"]');
    if (el) el.classList.toggle("active", on);
  }

  // ─── Multi-key movement ───
  const keysHeld = {
    w: false,
    a: false,
    s: false,
    d: false,
  };

  document.addEventListener("keydown", (e) => {
    if (e.repeat) return;
    const k = e.key.toLowerCase();
    if (k in keysHeld) {
      keysHeld[k] = true;
      setKey(k, true);
      updateMovement();
    }
  });

  document.addEventListener("keyup", (e) => {
    const k = e.key.toLowerCase();
    if (k in keysHeld) {
      keysHeld[k] = false;
      setKey(k, false);
      updateMovement();
    }
  });

  function updateMovement() {
    const w = keysHeld.w;
    const a = keysHeld.a;
    const s = keysHeld.s;
    const d = keysHeld.d;

    if (w && a)      send("/forwardleft",  "move_state");
    else if (w && d) send("/forwardright", "move_state");
    else if (s && a) send("/backleft",     "move_state");
    else if (s && d) send("/backright",    "move_state");
    else if (w)      send("/forward",      "move_state");
    else if (s)      send("/back",         "move_state");
    else if (a)      send("/left",         "move_state");
    else if (d)      send("/right",        "move_state");
    else             send("/stop",         "move_state");
  }

  // ─── Sliders ───
  const speedslider = document.getElementById("speed_slider");
  const speedOut = document.getElementById("speed_val");
  let speedTimer = null;
  speedslider.addEventListener("input", (e) => {
    speedOut.innerText = e.target.value;
    clearTimeout(speedTimer);
    speedTimer = setTimeout(() => {
      send("/speed?value=" + e.target.value, null);
    }, 80);
  });

  const turnslider = document.getElementById("turn_slider");
  const turnOut = document.getElementById("turn_val");
  let turnTimer = null;
  turnslider.addEventListener("input", (e) => {
    turnOut.innerText = e.target.value;
    clearTimeout(turnTimer);
    turnTimer = setTimeout(() => {
      send("/turnratio?value=" + e.target.value, null);
    }, 80);
  });

  // ─── Sensor data polling ───
  function updateSensorData() {
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        try {
          const data = JSON.parse(this.responseText);

          document.getElementById("ir_val").innerText = Math.round(data.ir);

          if (data.us) {
              document.getElementById("us_val").innerText = "40 kHz detected";
          } else {
              document.getElementById("us_val").innerText = "Silent";
          }

          const magEl = document.getElementById("mag_val");
          
          let tmp;
          if (data.mag >= 15) {
            tmp = "\u2191 UP";
          } else if (data.mag <= -15) {
            tmp = "\u2193 DOWN";
          } else {
            tmp = "NOT DETECTED"
          }

          magEl.innerText = String(data.mag) + "(" + tmp + ")";

          document.getElementById("age_val").innerText = data.age;

          const rockCard = document.getElementById("rock_card");
          const rockName = document.getElementById("rock_name");
          rockName.innerText = data.rock;
          rockCard.className = "rock-card " + data.rock;
        } catch (e) {}
      }
    };
    xhttp.open("GET", "/sensordata", true);
    xhttp.send();
  }

  setInterval(updateSensorData, 200);

  function ledOn()  { send("/on", "led_state"); }
  function ledOff() { send("/off", "led_state"); }

  // ─── Radio toggle ───
  // Sends GET /radiotoggle to the Arduino.
  // Arduino flips the flag and replies "1" (now on) or "0" (now off).
  // We use that reply to keep the button appearance in sync with reality —
  // even if the page was reloaded mid-session the button is always correct.
  function toggleRadio() {
    const btn = document.getElementById("radio_toggle");
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        const on = (this.responseText.trim() === "1");
        btn.classList.toggle("on", on);
      }
    };
    xhttp.open("GET", "/radiotoggle", true);
    xhttp.send();
  }
  </script>
  </body>
</html>
  )rawliteral";

WiFiWebServer server(80);

//Return the web page
void handleRoot() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send_P(200, PSTR("text/html"), PSTR(""));

  const char* p = webpage;
  size_t total = strlen_P(webpage);
  size_t sent = 0;
  const size_t chunkSize = 512;
  char buf[chunkSize + 1];

  // send in chunks
  while (sent < total) {
    size_t toSend = (total - sent < chunkSize) ? (total - sent) : chunkSize;
    memcpy_P(buf, p + sent, toSend);
    buf[toSend] = '\0';
    server.sendContent(buf);
    sent += toSend;
  }
}



void handleSpeed() {
  if (server.hasArg("value")) {
    int newSpeed = server.arg("value").toInt();
    if (newSpeed < 0) newSpeed = 0;
    if (newSpeed > 255) newSpeed = 255;
    speed = newSpeed;
  }
  server.send(200, F("text/plain"), String(speed));
}

void turnratio() {
  if (server.hasArg("value")) {
    float newturnratio = server.arg("value").toFloat();
    twoinputspeed = speed * newturnratio;
    Serial.println(twoinputspeed);
    Serial.println(speed);
    Serial.println(newturnratio);
  }
  server.send(200, F("text/plain"), String(twoinputspeed));
}

//Switch LED on and acknowledge
void ledON() {
  digitalWrite(LED_BUILTIN, 1);
  server.send(200, F("text/plain"), F("ON"));
}

//Switch LED on and acknowledge
void ledOFF() {
  digitalWrite(LED_BUILTIN, 0);
  server.send(200, F("text/plain"), F("OFF"));
}


void back() {
  digitalWrite(LEFT_DIR, LOW);
  analogWrite(LEFT_PWM, speed);
  digitalWrite(RIGHT_DIR, LOW);
  analogWrite(RIGHT_PWM, speed * right_speed_modifier);
  Serial.println("BACK");
  server.send(200, F("text/plain"), F("BACK"));
}


void forward() {
  digitalWrite(LEFT_DIR, HIGH);
  analogWrite(LEFT_PWM, speed);
  digitalWrite(RIGHT_DIR, HIGH);
  analogWrite(RIGHT_PWM, speed * right_speed_modifier);
  Serial.println("forward");
  server.send(200, F("text/plain"), F("FORWARD"));
}


void right() {
  digitalWrite(LEFT_DIR, LOW);
  analogWrite(LEFT_PWM, speed);
  digitalWrite(RIGHT_DIR, HIGH);
  analogWrite(RIGHT_PWM, speed * right_speed_modifier);
  Serial.println("right");
  server.send(200, F("text/plain"), F("RIGHT"));
}


void left() {
  digitalWrite(LEFT_DIR, HIGH);
  analogWrite(LEFT_PWM, speed);
  digitalWrite(RIGHT_DIR, LOW);
  analogWrite(RIGHT_PWM, speed * right_speed_modifier);
  Serial.println("left");
  server.send(200, F("text/plain"), F("LEFT"));
}

void stop() {
  analogWrite(LEFT_PWM, 0);
  analogWrite(RIGHT_PWM, 0);
  Serial.println("stop");
  server.send(200, F("text/plain"), F("STOP"));
}


//2-input controls
void forwardleft() {
  digitalWrite(LEFT_DIR, HIGH);
  analogWrite(LEFT_PWM, speed);
  digitalWrite(RIGHT_DIR, HIGH);
  analogWrite(RIGHT_PWM, twoinputspeed);
  Serial.println("forwardright");
  server.send(200, F("text/plain"), F("FORWARDLEFT"));
}


void forwardright() {
  digitalWrite(LEFT_DIR, HIGH);
  analogWrite(LEFT_PWM, twoinputspeed);
  digitalWrite(RIGHT_DIR, HIGH);
  analogWrite(RIGHT_PWM, speed);
  Serial.println("forwardleft");
  server.send(200, F("text/plain"), F("FORWARDRIGHT"));
}


void backleft() {
  digitalWrite(LEFT_DIR, LOW);
  analogWrite(LEFT_PWM, speed);
  digitalWrite(RIGHT_DIR, LOW);
  analogWrite(RIGHT_PWM, twoinputspeed * right_speed_modifier);
  Serial.println("backright");
  server.send(200, F("text/plain"), F("BACKLEFT"));
}


void backright() {
  digitalWrite(LEFT_DIR, LOW);
  analogWrite(LEFT_PWM, twoinputspeed);
  digitalWrite(RIGHT_DIR, LOW);
  analogWrite(RIGHT_PWM, speed * right_speed_modifier);
  Serial.println("backleft");
  server.send(200, F("text/plain"), F("BACKRIGHT"));
}


const char* classifyRock() {
  // Categorise each sensor reading into discrete buckets
  bool ir547 = (IRPulseRate > 400);    // true if IR is in 547 group

  // Match against the table
  if ( ir547 && magDir == -1 || magDir == -1 &&  ultraDetected || ir547 && ultraDetected) return "basaltoid";
  if (!ir547 && magDir == -1 || magDir == -1 && !ultraDetected || !ir547 && !ultraDetected) return "gravion";
  if (!ir547 && magDir == 1 || magDir == 1 &&  ultraDetected || !ir547 && ultraDetected) return "regolix";
  if ( ir547 && magDir == 1 || magDir == 1 && !ultraDetected || ir547 && !ultraDetected) return "lunarite";
  return "unknown";
}

void sendData() {
  String json = "{";
  json += "\"ir\":"    + String(IRPulseRate, 1) + ",";
  json += "\"mag\":"   + String(magReading)          + ",";   // ← was magReading
  json += "\"us\":"    + String(ultraDetected ? 1 : 0) + ",";  // ← bool not ADC
  json += "\"rock\":\"" + String(classifyRock()) + "\",";
  json += "\"age\":"   + rockAge;
  json += "}";
  Serial.println(json);
  server.send(200, F("application/json"), json);
}

// ─── Radio toggle handler ───
// Called when the web UI hits GET /radiotoggle.
// Flips the radioEnabled flag, then replies "1" (enabled) or "0" (disabled).
// The web UI uses this reply to keep the button appearance in sync.
void handleRadioToggle() {
  radioEnabled = !radioEnabled;
  Serial.print(F("Radio "));
  Serial.println(radioEnabled ? F("ENABLED") : F("DISABLED"));
  server.send(200, F("text/plain"), radioEnabled ? F("1") : F("0"));
}

//Generate a 404 response with details of the failed request
void handleNotFound() {
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? F("GET") : F("POST");
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, F("text/plain"), message);
}

void pulseDetected() {
  IRPulseCount++;
}



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

void updateIR() {
  int elapsedTime = millis() - lastIRTime;
  if (elapsedTime > IRSampleTime) {
    // Atomically grab and reset the count
    // noInterrupts();
    int count = IRPulseCount;
    IRPulseCount = 0;
    // interrupts();

    // pulses per second (×1000 because elapsedTime is in ms)
    IRPulseRate = (float) count * 1000.0 / (float) elapsedTime;
    // if (IRPulseRate > 1000) IRPulseRate = 0; // reject out of bounds values
    lastIRTime = millis();
  }

  
  Serial.print("IR: ");
  Serial.println(IRPulseRate);
}
void updateUltra() {
  int elapsedTime = millis() - lastUltraTime;
  digitalWrite(ULTRA_SWITCH, HIGH);
  delay(10);
  ultraReading = analogRead(A1);
  digitalWrite(ULTRA_SWITCH, LOW);
  if (elapsedTime > ultraSampleTime) {
    lastUltraTime = millis();

    if (abs(ultraMax - ultraMin) > ULTRA_THRES) {
      ultraDetected = true;
    } else {
      ultraDetected = false;
    }
    ultraMin = 1023;
    ultraMax = 0;
  }

  if (ultraReading > ultraMax) ultraMax = ultraReading;
  if (ultraReading < ultraMin) ultraMin = ultraReading;

}

void updateMag() {
  magReading = analogRead(A0) - MAG_STABLE_VAL;
  Serial.print("mag: ");
  Serial.println(analogRead(A0));
  if (magReading > MAG_THRES) {
    magDir = 1;
  } else if (magReading < -MAG_THRES) {
    magDir = -1;
  } else {
    magDir = 0;
  }

}

void updateRadio() {
  // Wait for idle, then for a start bit
  unsigned long idleWait = millis() + 5;
  while (!lineIsIdle()) {
    if (millis() > idleWait) return;
  }

  unsigned long startWait = millis() + 50;
  while (lineIsIdle()) {
    if (millis() > startWait) return;
  }

  // Read one byte inline
  delayMicroseconds(HALF_BIT_US);
  if (lineIsIdle()) return;

  byte b = 0;
  for (int i = 0; i < 8; i++) {
    delayMicroseconds(BIT_PERIOD_US);
    if (lineIsIdle()) b |= (1 << i);
  }
  delayMicroseconds(BIT_PERIOD_US);  // stop bit

  // If it's not '#', not interested
  if ((char)b != '#') return;

  // It IS '#' — read remaining 3 bytes immediately
  int x = readByte(200);
  int y = readByte(200);
  int z = readByte(200);

  if (x < 0 || y < 0 || z < 0) {
    Serial.println("(timeout mid-frame)");
    return;
  }

  if (isDigit(x) && isDigit(y) && isDigit(z)) {
    String tmp = String((char)x);
    tmp += ".";
    tmp += String((char)y);
    tmp += String((char)z);
    rockAge = tmp;
    Serial.print("Rock age = ");
    Serial.print(rockAge);
    Serial.println(" billion years");
  } else {
    Serial.println("(frame error)");
  }
}



void setup() {
  pinMode(ULTRA_SWITCH, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  pinMode(LEFT_DIR, OUTPUT);
  pinMode(LEFT_PWM, OUTPUT);
  pinMode(RIGHT_DIR, OUTPUT);
  pinMode(RIGHT_PWM, OUTPUT);


  pinMode(RX_PIN, INPUT);

  pinMode(IR_INPUT, INPUT);
  attachInterrupt(digitalPinToInterrupt(IR_INPUT),
                  pulseDetected,
                  RISING);

  Serial.begin(115200);

  //Wait 10s for the serial connection before proceeding
  //This ensures you can see messages from startup() on the monitor
  //Remove this for faster startup when the USB host isn't attached
  while (!Serial && millis() < 10000)
    ;

  Serial.println(F("\nStarting Web Server"));

  //Check WiFi shield is present
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("WiFi shield not present"));
    while (true)
      ;
  }


  //Configure the static IP address if group number is set
  if (groupNumber)
    WiFi.config(IPAddress(192, 168, 0, groupNumber + 1));

  // attempt to connect to WiFi network
  Serial.print(F("Connecting to WPA SSID: "));
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }

  //Register the callbacks to respond to HTTP requests
  server.on(F("/"), handleRoot);
  server.on(F("/on"), ledON);
  server.on(F("/off"), ledOFF);
  server.on(F("/forward"), forward);
  server.on(F("/left"), left);
  server.on(F("/right"), right);
  server.on(F("/back"), back);
  server.on(F("/stop"), stop);
  server.on(F("/speed"), handleSpeed);
  server.on(F("/sensordata"), sendData);
  server.on(F("/radiotoggle"), handleRadioToggle);   // ← new

  //2-input callbacks
  server.on(F("/turnratio"), turnratio);
  server.on(F("/forwardright"), forwardright);
  server.on(F("/forwardleft"), forwardleft);
  server.on(F("/backright"), backright);
  server.on(F("/backleft"), backleft);


  server.onNotFound(handleNotFound);

  server.begin();

  Serial.print(F("HTTP server started @ "));
  Serial.println(static_cast<IPAddress>(WiFi.localIP()));
}

//Call the server polling function in the main loop
void loop() {
  // Drain all pending web requests for up to 20ms before doing anything else
  unsigned long serveUntil = millis() + 20;
  while (millis() < serveUntil) {
    server.handleClient();
  }

  updateIR();
  // delay(100);
  updateUltra();
  updateMag();



  if (radioEnabled) {
    updateRadio();
  }


}
