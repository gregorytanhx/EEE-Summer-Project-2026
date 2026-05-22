
// #define USE_HOTSPOT
#define USE_WIFI_NINA false
#define USE_WIFI101 true
#include <WiFiWebServer.h>

#define LEFT_DIR 8
#define LEFT_PWM 9
#define RIGHT_DIR 3
#define RIGHT_PWM 4

#ifdef USE_HOTSPOT
const int groupNumber = 0;
const char ssid[] = "hotspot";
const char pass[] = "freew1f1";

#else

const int groupNumber = 24;  // Set your group number to make the IP address constant - only do this on the EEERover network
const char ssid[] = "EEERover";
const char pass[] = "exhibition";
#endif

int speed = 255;
int twoinputspeed = speed;


//TODO: add speed slider

//Webpage to return when root is requested
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
        max-width: 420px;
      }
      h1 {
        font-size: 13px;
        letter-spacing: 0.25em;
        color: #ffb547;
        padding-bottom: 14px;
        border-bottom: 1px solid #222;
        margin: 0 0 24px;
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
      #led_state, #move_state, #speed_val {
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
        margin-top: 20px;
        padding-top: 16px;
        border-top: 1px dashed #222;
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
      .speed {
        margin-top: 24px;
        padding-top: 16px;
        border-top: 1px dashed #222;
      }
      .speed-head {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: 10px;
      }
      .turnratio {
        margin-top: 24px;
        padding-top: 16px;
        border-top: 1px dashed #222;
      }
      .turnratio-head {
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
    </style>
  </head>
  <body>
    <h1>LUNAR &middot; ROVER</h1>
    <button class="btn" onclick="ledOn()">LED On</button>
    <button class="btn" onclick="ledOff()">LED Off</button>

    <div class="status">
      <span class="label">LED State:</span> <span id="led_state">OFF</span>
      <br>
      <span class="label">Movement:</span> <span id="move_state">STOP</span>
    </div>

    <div class="pad">
      <span class="label">Controls</span>
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
    <br>

    <div class="turnratio">
      <div class="turnratio-head">
        <span class="label">Turning Ratio</span>
        <span id="turn_val">1</span>
      </div>
      <input type="range" id="turn_slider" min="0" max="1" step="0.01" value="1">
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

  const keysHeld = {
    w: false,
    a: false,
    s: false,
    d: false,
  }
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
    else if (s && a) send("/backleft",  "move_state");
    else if (s && d) send("/backright", "move_state");
    else if (w)      send("/forward",      "move_state");
    else if (s)      send("/back",      "move_state");
    else if (a)      send("/left",         "move_state");
    else if (d)      send("/right",        "move_state");
    else             send("/stop",         "move_state");
  }

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


  function ledOn()  { send("/on", "led_state"); }
  function ledOff() { send("/off", "led_state"); }
  function forward() { send("/forward", "move_state"); }
  function back() { send("/back", "move_state"); }
  function left()    { send("/left", "move_state"); }
  function right()   { send("/right", "move_state"); }
  function stop()    { send("/stop", "move_state"); }

  function forwardright() { send("/forwardright", "move_state"); }
  function forwardleft() { send("/forwardleft", "move_state"); }
  function backright() { send("/backright", "move_state"); }
  function backleft() { send("/backleft", "move_state"); }

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
  
  while (sent < total) {
    size_t toSend = (total - sent < chunkSize) ? (total - sent) : chunkSize;
    memcpy_P(buf, p + sent, toSend);
    buf[toSend] = '\0';
    server.sendContent(buf);
    sent += toSend;
  }
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

void handleSpeed() {
  if (server.hasArg("value")) {
    int newSpeed = server.arg("value").toInt();
    if (newSpeed < 0) newSpeed = 0;
    if (newSpeed > 255) newSpeed = 255;
    speed = newSpeed;
  }
  server.send(200, F("text/plain"), String(speed));
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
  analogWrite(RIGHT_PWM, speed);
  Serial.println("BACK");
  server.send(200, F("text/plain"), F("BACK"));
}


void forward() {
  digitalWrite(LEFT_DIR, HIGH);
  analogWrite(LEFT_PWM, speed);
  digitalWrite(RIGHT_DIR, HIGH);
  analogWrite(RIGHT_PWM, speed);
  Serial.println("forward");
  server.send(200, F("text/plain"), F("FORWARD"));
}


void right() {
  digitalWrite(LEFT_DIR, LOW);
  analogWrite(LEFT_PWM, speed);
  digitalWrite(RIGHT_DIR, HIGH);
  analogWrite(RIGHT_PWM, speed);
  Serial.println("right");
  server.send(200, F("text/plain"), F("RIGHT"));
}


void left() {
  digitalWrite(LEFT_DIR, HIGH);
  analogWrite(LEFT_PWM, speed);
  digitalWrite(RIGHT_DIR, LOW);
  analogWrite(RIGHT_PWM, speed);
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
void forwardright() {
  digitalWrite(LEFT_DIR, HIGH);
  analogWrite(LEFT_PWM, twoinputspeed);
  digitalWrite(RIGHT_DIR, HIGH);
  analogWrite(RIGHT_PWM, speed);
  Serial.println("forwardright");
  server.send(200, F("text/plain"), F("FORWARDRIGHT"));
}


void forwardleft() {
  digitalWrite(LEFT_DIR, HIGH);
  analogWrite(LEFT_PWM, speed);
  digitalWrite(RIGHT_DIR, HIGH);
  analogWrite(RIGHT_PWM, twoinputspeed);
  Serial.println("forwardleft");
  server.send(200, F("text/plain"), F("FORWARDLEFT"));
}


void backright() {
  digitalWrite(LEFT_DIR, LOW);
  analogWrite(LEFT_PWM, twoinputspeed);
  digitalWrite(RIGHT_DIR, LOW);
  analogWrite(RIGHT_PWM, speed);
  Serial.println("backright");
  server.send(200, F("text/plain"), F("BACKRIGHT"));
}


void backleft() {
  digitalWrite(LEFT_DIR, LOW);
  analogWrite(LEFT_PWM, speed);
  digitalWrite(RIGHT_DIR, LOW);
  analogWrite(RIGHT_PWM, twoinputspeed);
  Serial.println("backleft");
  server.send(200, F("text/plain"), F("BACKLEFT"));
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

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);
  pinMode(LEFT_DIR, OUTPUT);
  pinMode(LEFT_PWM, OUTPUT);
  pinMode(RIGHT_DIR, OUTPUT);
  pinMode(RIGHT_PWM, OUTPUT);
  

  Serial.begin(9600);

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
  server.handleClient();
}