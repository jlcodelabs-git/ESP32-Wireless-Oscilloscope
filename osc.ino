/*
 * Wireless Dual-Channel Oscilloscope for  XIAO ESP32-C3 
 * 
 * Features:
 * - Dual analog input channels (GPIO1/A0 and GPIO2/A1)
 * - WiFi transmission for high-speed data transfer
 * - Real-time sampling with configurable sample rate
 * - JSON data format for easy parsing
 * - Push button cycles waveform generator (SINE, SQUARE, SAWTOOTH, TRIANGLE)
 * - WebSocket command can also change waveform (WAVE:SINE, WAVE:SQUARE, etc.)
 * 
 * Hardware Setup:
 * - Channel 1: GPIO1 (A0) - Connect to input amplifier circuit
 * - Channel 2: GPIO2 (A1) - Connect to input amplifier circuit
 * - Input voltage range: 0-3.3V (use voltage divider for higher voltages)
 * - DAC/Signal Out: DAC_PIN (PWM with RC filter recommended)
 * - Button: BUTTON_PIN to GND (internal pull-up used)


 Seeed Studio XIAO ESP32-C3 
The usable pins are much fewer. Here’s the correct pinout for analog (ADC) use:
	•	ADC-capable pins:
	•	A0 → GPIO2
	•	A1 → GPIO3
	•	A2 → GPIO4
	•	A3 → GPIO5
	•	A4 → GPIO6
	•	A5 → GPIO7

 */



#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <math.h>   // for sin()

// Board Configuration
#define LED_PIN D6          // Built-in LED
#define CHANNEL_1_PIN D0     // GPIO1 (A0) - First oscilloscope channel
#define CHANNEL_2_PIN D2    // GPIO3 (A1) - Second oscilloscope channel

// --- NEW DEFINES FOR WAVEFORM GENERATOR ---
#define BUTTON_PIN D5       // GPIO for push button
#define DAC_PIN D4           // GPIO for PWM output

// Waveform modes
enum Waveform { SINE, SQUARE, SAW, TRIANGLE };
Waveform currentWaveform = SINE;

// Button state tracking
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // ms

// WiFi Configuration
const char* ssid = "Wifi";        // Replace with your WiFi network

const char* password = "password";   // Replace with your WiFi password

// Oscilloscope Configuration
const int SAMPLE_RATE = 1000;           // Samples per second (adjustable)
const int BUFFER_SIZE = 500;            // Number of samples per transmission
const int ADC_RESOLUTION = 4095;        // 12-bit ADC (0-4095)
const float VOLTAGE_REF = 3.3;          // Reference voltage

// Global Variables
WebSocketsServer webSocket = WebSocketsServer(81);
bool isConnected = false;
bool isRunning = false;
unsigned long lastSampleTime = 0;
int sampleBuffer1[BUFFER_SIZE];
int sampleBuffer2[BUFFER_SIZE];
int bufferIndex = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // LED on during setup
  
  // Initialize ADC pins
  pinMode(CHANNEL_1_PIN, INPUT);
  pinMode(CHANNEL_2_PIN, INPUT);

  // Initialize Button and DAC
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(DAC_PIN, OUTPUT);
  
  // Set ADC resolution to 12 bits (0-4095)
  analogReadResolution(12);
  
  // Connect to WiFi
  Serial.println("Starting WiFi Oscilloscope...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // Blink LED while connecting
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
  
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("WebSocket server started on port 81");
  
  // Start WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  digitalWrite(LED_PIN, LOW); // LED off when ready
  Serial.println("Oscilloscope ready! Connect your computer app to:");
  Serial.print("ws://");
  Serial.print(WiFi.localIP());
  Serial.println(":81");
}

void loop() {
  webSocket.loop();

  // --- Handle button press for waveform cycling ---
  bool buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW && lastButtonState == HIGH && (millis() - lastDebounceTime) > debounceDelay) {
    lastDebounceTime = millis();
    currentWaveform = (Waveform)((currentWaveform + 1) % 4);
    Serial.print("Waveform changed to: ");
    Serial.println(currentWaveform);
    sendWaveformUpdate();
  }
  lastButtonState = buttonState;

  // Generate waveform output
  generateWaveform(currentWaveform);

  // Keep oscilloscope running
  if (isConnected && isRunning) {
    if (micros() - lastSampleTime >= (1000000 / SAMPLE_RATE)) {
      lastSampleTime = micros();
      sampleBuffer1[bufferIndex] = analogRead(CHANNEL_1_PIN);
      sampleBuffer2[bufferIndex] = analogRead(CHANNEL_2_PIN);
      bufferIndex++;
      if (bufferIndex >= BUFFER_SIZE) {
        sendSampleData();
        bufferIndex = 0;
      }
    }
  }

  // Blink LED when connected and running
  if (isConnected && isRunning) {
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 1000) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastBlink = millis();
    }
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      isConnected = false;
      isRunning = false;
      digitalWrite(LED_PIN, LOW);
      break;
      
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      isConnected = true;
      
      // Send initial configuration
      sendConfiguration();
      break;
    }
    
    case WStype_TEXT: {
      Serial.printf("[%u] Received: %s\n", num, payload);
      handleCommand((char*)payload);
      break;
    }
      
    default:
      break;
  }
}

void handleCommand(String command) {
  if (command == "START") {
    isRunning = true;
    bufferIndex = 0;
    Serial.println("Oscilloscope started");
  }
  else if (command == "STOP") {
    isRunning = false;
    Serial.println("Oscilloscope stopped");
  }
  else if (command.startsWith("RATE:")) {
    int newRate = command.substring(5).toInt();
    if (newRate >= 10 && newRate <= 10000) {
      Serial.println("Sample rate updated to: " + String(newRate) + " Hz");
    }
  }
  else if (command.startsWith("WAVE:")) {
    String waveType = command.substring(5);
    waveType.toUpperCase();
    if (waveType == "SINE") currentWaveform = SINE;
    else if (waveType == "SQUARE") currentWaveform = SQUARE;
    else if (waveType == "SAW" || waveType == "SAWTOOTH") currentWaveform = SAW;
    else if (waveType == "TRIANGLE") currentWaveform = TRIANGLE;
    Serial.println("Waveform set by WebSocket to: " + waveType);
    sendWaveformUpdate();
  }
}

void sendConfiguration() {
  DynamicJsonDocument doc(300);
  doc["type"] = "config";
  doc["channels"] = 2;
  doc["sampleRate"] = SAMPLE_RATE;
  doc["bufferSize"] = BUFFER_SIZE;
  doc["voltageRef"] = VOLTAGE_REF;
  doc["adcResolution"] = ADC_RESOLUTION;

  switch (currentWaveform) {
    case SINE: doc["waveform"] = "SINE"; break;
    case SQUARE: doc["waveform"] = "SQUARE"; break;
    case SAW: doc["waveform"] = "SAWTOOTH"; break;
    case TRIANGLE: doc["waveform"] = "TRIANGLE"; break;
  }

  String jsonString;
  serializeJson(doc, jsonString);
  webSocket.broadcastTXT(jsonString);
}

void sendSampleData() {
  DynamicJsonDocument doc(4000);
  doc["type"] = "data";
  doc["timestamp"] = millis();
  
  JsonArray ch1 = doc.createNestedArray("channel1");
  JsonArray ch2 = doc.createNestedArray("channel2");
  
  for (int i = 0; i < BUFFER_SIZE; i++) {
    float voltage1 = (sampleBuffer1[i] * VOLTAGE_REF) / ADC_RESOLUTION;
    float voltage2 = (sampleBuffer2[i] * VOLTAGE_REF) / ADC_RESOLUTION;
    ch1.add(voltage1);
    ch2.add(voltage2);
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  webSocket.broadcastTXT(jsonString);
  
  Serial.print("Sent ");
  Serial.print(BUFFER_SIZE);
  Serial.println(" samples");
}

// --- NEW FUNCTION: Send waveform updates ---
void sendWaveformUpdate() {
  DynamicJsonDocument doc(200);
  doc["type"] = "waveform";
  switch (currentWaveform) {
    case SINE: doc["value"] = "SINE"; break;
    case SQUARE: doc["value"] = "SQUARE"; break;
    case SAW: doc["value"] = "SAWTOOTH"; break;
    case TRIANGLE: doc["value"] = "TRIANGLE"; break;
  }
  String jsonString;
  serializeJson(doc, jsonString);
  webSocket.broadcastTXT(jsonString);
}

// --- NEW FUNCTION: Waveform generator ---
void generateWaveform(Waveform type) {
  static int phase = 0;
  const int amplitude = 255;  // 8-bit PWM
  const int steps = 100;      // resolution
  static unsigned long lastUpdate = 0;

  if (micros() - lastUpdate < 100) return; // ~10 kHz update
  lastUpdate = micros();

  int value = 0;
  switch (type) {
    case SINE:
      value = (sin(2 * PI * phase / steps) * 0.5 + 0.5) * amplitude;
      break;
    case SQUARE:
      value = (phase < steps/2) ? amplitude : 0;
      break;
    case SAW:
      value = (phase * amplitude) / steps;
      break;
    case TRIANGLE:
      value = (phase < steps/2) ? (phase * 2 * amplitude / steps)
                                : (amplitude - (phase - steps/2) * 2 * amplitude / steps);
      break;
  }

  analogWrite(DAC_PIN, value); // PWM output
  phase = (phase + 1) % steps;
}
