// ===================================================
// ESP32 Single Joystick Car (Offline Access Point)
// L298N Motor Driver + WebSocket Control
// Final Enhanced Code by Saffron 🚩(49th Trial)
// ===================================================

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// ========================
// Access Point credentials
const char* ssid = "ESP32_WEBCAR_AP";
const char* password = "Saffron@";

// ========================
// Web Server & WebSocket
WebServer server(80);
WebSocketsServer webSocket(81);

// ========================
// Motor pins (L298N + ESP32)
#define IN1 26
#define IN2 25
#define IN3 33
#define IN4 32
#define ENA 27
#define ENB 14

// ========================
// Variables
int leftPWM = 0;
int rightPWM = 0;

// ========================
// Movement functions
void stopCar() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

void moveForward(int lpwm, int rpwm) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, lpwm);
  analogWrite(ENB, rpwm);
}

void moveBackward(int lpwm, int rpwm) {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, lpwm);
  analogWrite(ENB, rpwm);
}

void turnLeft(int lpwm, int rpwm) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, lpwm / 2);   // slower left motor
  analogWrite(ENB, rpwm);       // full right motor
}

void turnRight(int lpwm, int rpwm) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, lpwm);       // full left motor
  analogWrite(ENB, rpwm / 2);   // slower right motor
}

// ========================
// WebSocket handler
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if(type == WStype_TEXT){
    String msg = String((char*)payload);
    Serial.println(msg);

    if(msg.startsWith("forward")){
      sscanf(msg.c_str(),"forward:%d:%d",&leftPWM,&rightPWM);
      moveForward(leftPWM,rightPWM);
    } else if(msg.startsWith("backward")){
      sscanf(msg.c_str(),"backward:%d:%d",&leftPWM,&rightPWM);
      moveBackward(leftPWM,rightPWM);
    } else if(msg.startsWith("turnLeft")){
      sscanf(msg.c_str(),"turnLeft:%d:%d",&leftPWM,&rightPWM);
      turnLeft(leftPWM,rightPWM);
    } else if(msg.startsWith("turnRight")){
      sscanf(msg.c_str(),"turnRight:%d:%d",&leftPWM,&rightPWM);
      turnRight(leftPWM,rightPWM);
    } else if(msg.startsWith("stop")){
      stopCar();
    }
  }
}

// ========================
// Updated HTML Page
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>🚗 ESP32 Spicy Drive</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body { text-align:center;font-family:'Poppins',sans-serif;background:radial-gradient(circle at center,#050505,#0b0b0b 70%);color:#00eaff;overflow:hidden; }
  h2 { margin-top:10px;font-size:26px;color:#00eaff;text-shadow:0 0 15px #00eaff; }
  #wifiStatus { margin-top:5px;font-size:14px;color:#00ffaa;text-shadow:0 0 10px #00ffaa; }
  .joystick { width:180px;height:180px;background:#1a1a1a;border-radius:50%;position:relative;margin:40px auto;touch-action:none;box-shadow:0 0 20px rgba(0,234,255,0.4);user-select:none; }
  .stick { width:70px;height:70px;background:linear-gradient(145deg,#00eaff,#0088ff);border-radius:50%;position:absolute;top:55px;left:55px;box-shadow:0 0 25px #00eaff;transition:0.05s; }
  #stopBtn { padding:15px 50px;border:none;border-radius:12px;background:linear-gradient(90deg,#ff0044,#ff6600);color:white;font-size:20px;cursor:pointer;box-shadow:0 0 25px rgba(255,60,0,0.8);margin:30px auto;transition:0.25s; }
  #stopBtn:hover { transform:scale(1.08);background:#ff0000; }
  .speed-control { margin-top:10px; }
  .speed-btn { padding:14px 28px;margin:10px;border:none;border-radius:8px;background:#1b1b1b;color:#00eaff;font-size:17px;cursor:pointer;box-shadow:0 0 12px rgba(0,234,255,0.3);transition:all 0.25s; }
  .speed-btn:hover { background:#00eaff;color:#111; transform:scale(1.05); }
  .speed-btn.active { background:#00eaff;color:#111;box-shadow:0 0 20px #00eaff; }
  #footer { position:fixed;bottom:0;width:100%;background:linear-gradient(90deg,#00eaff,#0066ff);color:#fff;font-weight:bold;padding:10px 0;font-family:'Courier New',monospace;box-shadow:0 -3px 15px rgba(0,234,255,0.6);text-align:center;letter-spacing:1px; }
</style>
</head>
<body>

<h2>🌶️ ESP32 Spicy Drive Controller</h2>
<div id="wifiStatus">Wi-Fi Status: Connecting...</div>

<div class="joystick" id="joystick"><div class="stick" id="stick"></div></div>

<div class="speed-control">
  <button class="speed-btn active" data-speed="0.7">🌶️ Spicy Western</button>
  <button class="speed-btn" data-speed="1.0">🔥 Spicy Indian</button>
</div>

<button id="stopBtn">🛑 STOP</button>

<div id="footer">Code by Saffron 🚩</div>

<script>
const joystick = document.getElementById("joystick");
const stick = document.getElementById("stick");
const stopBtn = document.getElementById("stopBtn");
const speedBtns = document.querySelectorAll(".speed-btn");
const wifiStatus = document.getElementById("wifiStatus");

let center = 90, active = false, xVal = 0, yVal = 0;
let speedMultiplier = 0.7, ws;

// ✅ WebSocket Connection with Auto Reconnect
function connectWS(){
  ws = new WebSocket('ws://' + location.host + ':81/');
  ws.onopen = () => {
    wifiStatus.innerHTML = "Wi-Fi Status: Connected ✅";
    wifiStatus.style.color = "#00ffaa";
  };
  ws.onclose = () => {
    wifiStatus.innerHTML = "Wi-Fi Status: Disconnected ❌ Reconnecting...";
    wifiStatus.style.color = "#ff5555";
    setTimeout(connectWS, 2000);
  };
  ws.onerror = () => { ws.close(); };
}
connectWS();

// ✅ Speed Mode Buttons
speedBtns.forEach(btn => {
  btn.addEventListener("click", () => {
    speedBtns.forEach(b => b.classList.remove("active"));
    btn.classList.add("active");
    speedMultiplier = parseFloat(btn.dataset.speed);
  });
});

// ✅ Joystick Movement
function moveStick(clientX, clientY){
  const rect = joystick.getBoundingClientRect();
  xVal = clientX - rect.left - center;
  yVal = clientY - rect.top - center;

  const dist = Math.sqrt(xVal*xVal + yVal*yVal);
  if(dist > 70){
    const angle = Math.atan2(yVal, xVal);
    xVal = Math.cos(angle)*70;
    yVal = Math.sin(angle)*70;
  }

  stick.style.left = (xVal + 55) + "px";
  stick.style.top  = (yVal + 55) + "px";

  sendCommand();
}

// ✅ Updated, Error-Free Function
function sendCommand(){
  if(!ws || ws.readyState !== 1) return;

  let forward = -yVal / 70.0;
  let turn = xVal / 70.0;

  let leftMotor = forward + turn * 0.5;
  let rightMotor = forward - turn * 0.5;

  leftMotor = Math.max(-1, Math.min(1, leftMotor));
  rightMotor = Math.max(-1, Math.min(1, rightMotor));

  let leftPWM = Math.abs(Math.floor(leftMotor * 255 * speedMultiplier));
  let rightPWM = Math.abs(Math.floor(rightMotor * 255 * speedMultiplier));

  if(Math.abs(forward) < 0.1 && Math.abs(turn) < 0.1){
    ws.send("stop:0");
  } else if(forward > 0.1){
    ws.send("forward:" + leftPWM + ":" + rightPWM);
  } else if(forward < -0.1){
    ws.send("backward:" + leftPWM + ":" + rightPWM);
  } else if(turn > 0.2){
    ws.send("turnRight:" + leftPWM + ":" + rightPWM);
  } else if(turn < -0.2){
    ws.send("turnLeft:" + leftPWM + ":" + rightPWM);
  }
}

// ✅ Reset Function
function resetStick(){
  stick.style.left = "55px";
  stick.style.top = "55px";
  xVal = 0; yVal = 0;
}

// ✅ Stop Button
stopBtn.addEventListener("click", () => {
  if(ws && ws.readyState === 1) ws.send("stop:0");
  resetStick();
});

// ✅ Joystick Pointer Controls
joystick.addEventListener("pointerdown", e => {
  active = true;
  moveStick(e.clientX, e.clientY);
  joystick.setPointerCapture(e.pointerId);
});
joystick.addEventListener("pointermove", e => {
  if(active) moveStick(e.clientX, e.clientY);
});
joystick.addEventListener("pointerup", e => {
  active = false; resetStick(); sendCommand();
});
joystick.addEventListener("pointercancel", e => {
  active = false; resetStick(); sendCommand();
});
</script>
</body>
</html>
)rawliteral";

// ========================
// Setup
void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);

  stopCar();

  WiFi.softAP(ssid, password);
  Serial.println("Access Point started!");
  Serial.print("SSID: "); Serial.println(ssid);
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  server.on("/", []() { server.send(200, "text/html", HTML_PAGE); });
  server.begin();

  Serial.println("Web Server & WebSocket running...");
}

// ========================
// Loop
void loop() {
  webSocket.loop();
  server.handleClient();
}
// ========================
// Code by Saffron 🚩
// ========================