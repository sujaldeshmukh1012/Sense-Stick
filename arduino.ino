#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <SPI.h>

// WiFi credentials
const char* ssid = "admin";
const char* password = "admin123";

// API endpoints
const char* baseURL = "https://example.com";
const char* userID = "USER_001"; // Change this to actual user ID
String dataEndpoint = String(baseURL) + "/" + userID + "/data/";
String sosEndpoint = String(baseURL) + "/" + userID + "/SOS";

// Pin definitions for Arduino MEGA
// ESP32 Communication
#define ESP32_RX 2
#define ESP32_TX 3

// Sensors and actuators on MEGA
#define TRIG_PIN 4
#define ECHO_PIN 5
#define VIBRATION_MOTOR 6
#define BUZZER_PIN 7
#define POWER_BUTTON 8
#define PANIC_BUTTON 9
#define SD_CS_PIN 10

// ESP32 Pin definitions (when programming ESP32 separately)
// Camera pins (ESP32-CAM)
#define CAM_POWER 32
#define MIC_PIN 33
#define GPS_RX 16
#define GPS_TX 17

// Software Serial for communication
SoftwareSerial esp32Serial(ESP32_RX, ESP32_TX);
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);

// Global variables
bool systemEnabled = false;
bool panicPressed = false;
unsigned long lastDataSend = 0;
unsigned long lastSensorRead = 0;
const unsigned long DATA_SEND_INTERVAL = 30000; // Send data every 30 seconds
const unsigned long SENSOR_READ_INTERVAL = 200; // Read sensor every 200ms

// Distance thresholds (in cm)
const int CRITICAL_DISTANCE = 30;  // Immediate danger
const int WARNING_DISTANCE = 60;   // Warning zone
const int SAFE_DISTANCE = 100;     // Safe zone

// GPS data structure
struct GPSData {
  float latitude;
  float longitude;
  bool valid;
};

// Function prototypes
void setupWiFi();
void setupPins();
void setupSD();
void handleUltrasonicSensor();
void handlePanicButton();
void sendDataToServer();
void sendSOSSignal();
void alertUser(int distance);
void captureAndSendData();
GPSData readGPS();
String captureImage();
String recordAudio();

void setup() {
  Serial.begin(115200);
  esp32Serial.begin(115200);
  gpsSerial.begin(9600);
  
  Serial.println("SENSE Stick Initializing...");
  
  setupPins();
  setupSD();
  setupWiFi();
  
  Serial.println("SENSE Stick Ready!");
  
  // Initial vibration to indicate system ready
  digitalWrite(VIBRATION_MOTOR, HIGH);
  delay(500);
  digitalWrite(VIBRATION_MOTOR, LOW);
}

void loop() {
  // Check power button
  if (digitalRead(POWER_BUTTON) == LOW) {
    systemEnabled = !systemEnabled;
    delay(500); // Debounce
    
    if (systemEnabled) {
      Serial.println("System ENABLED");
      // Two short vibrations for enabled
      for (int i = 0; i < 2; i++) {
        digitalWrite(VIBRATION_MOTOR, HIGH);
        delay(200);
        digitalWrite(VIBRATION_MOTOR, LOW);
        delay(200);
      }
    } else {
      Serial.println("System DISABLED");
      // One long vibration for disabled
      digitalWrite(VIBRATION_MOTOR, HIGH);
      delay(1000);
      digitalWrite(VIBRATION_MOTOR, LOW);
    }
  }
  
  if (systemEnabled) {
    // Handle ultrasonic sensor for obstacle detection
    if (millis() - lastSensorRead > SENSOR_READ_INTERVAL) {
      handleUltrasonicSensor();
      lastSensorRead = millis();
    }
    
    // Handle panic button
    handlePanicButton();
    
    // Send data periodically
    if (millis() - lastDataSend > DATA_SEND_INTERVAL) {
      sendDataToServer();
      lastDataSend = millis();
    }
  }
  
  delay(50); // Small delay to prevent overwhelming the system
}

void setupPins() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(VIBRATION_MOTOR, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(POWER_BUTTON, INPUT_PULLUP);
  pinMode(PANIC_BUTTON, INPUT_PULLUP);
  pinMode(CAM_POWER, OUTPUT);
  
  digitalWrite(VIBRATION_MOTOR, LOW);
  digitalWrite(CAM_POWER, HIGH); // Power on camera
}

void setupSD() {
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card initialization failed!");
    return;
  }
  Serial.println("SD Card initialized successfully");
}

void setupWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi connection failed!");
  }
}

void handleUltrasonicSensor() {
  // Trigger ultrasonic sensor
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read echo
  long duration = pulseIn(ECHO_PIN, HIGH);
  int distance = duration * 0.034 / 2; // Convert to cm
  
  if (distance > 0 && distance < 300) { // Valid reading
    alertUser(distance);
  }
}

void alertUser(int distance) {
  if (distance <= CRITICAL_DISTANCE) {
    // Critical alert - continuous vibration and rapid beeping
    digitalWrite(VIBRATION_MOTOR, HIGH);
    for (int i = 0; i < 5; i++) {
      tone(BUZZER_PIN, 2000, 100);
      delay(100);
      noTone(BUZZER_PIN);
      delay(50);
    }
    digitalWrite(VIBRATION_MOTOR, LOW);
  } 
  else if (distance <= WARNING_DISTANCE) {
    // Warning alert - intermittent vibration and slower beeping
    digitalWrite(VIBRATION_MOTOR, HIGH);
    delay(200);
    digitalWrite(VIBRATION_MOTOR, LOW);
    tone(BUZZER_PIN, 1500, 300);
    delay(500);
    noTone(BUZZER_PIN);
  }
  else if (distance <= SAFE_DISTANCE) {
    // Gentle alert - short vibration and low tone
    digitalWrite(VIBRATION_MOTOR, HIGH);
    delay(100);
    digitalWrite(VIBRATION_MOTOR, LOW);
    tone(BUZZER_PIN, 1000, 200);
    delay(300);
    noTone(BUZZER_PIN);
  }
}

void handlePanicButton() {
  if (digitalRead(PANIC_BUTTON) == LOW) {
    if (!panicPressed) {
      panicPressed = true;
      Serial.println("PANIC BUTTON PRESSED!");
      sendSOSSignal();
      
      // Panic feedback - alternating vibration and sound
      for (int i = 0; i < 3; i++) {
        digitalWrite(VIBRATION_MOTOR, HIGH);
        tone(BUZZER_PIN, 3000, 500);
        delay(500);
        digitalWrite(VIBRATION_MOTOR, LOW);
        noTone(BUZZER_PIN);
        delay(200);
      }
    }
  } else {
    panicPressed = false;
  }
}

void sendDataToServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Reconnecting...");
    setupWiFi();
    return;
  }
  
  Serial.println("Sending data to server...");
  
  // Get GPS data
  GPSData gpsData = readGPS();
  
  // Capture image and audio
  String imageData = captureImage();
  String audioData = recordAudio();
  
  // Create JSON payload
  StaticJsonDocument<1024> jsonDoc;
  jsonDoc["timestamp"] = millis();
  jsonDoc["user_id"] = userID;
  
  if (gpsData.valid) {
    jsonDoc["gps"]["latitude"] = gpsData.latitude;
    jsonDoc["gps"]["longitude"] = gpsData.longitude;
  }
  
  jsonDoc["camera"]["image_data"] = imageData;
  jsonDoc["microphone"]["audio_data"] = audioData;
  jsonDoc["system"]["battery_level"] = 85; // You can add actual battery monitoring
  jsonDoc["system"]["device_status"] = "active";
  
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  
  // Send HTTP POST request
  HTTPClient http;
  http.begin(dataEndpoint);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Server response: " + response);
  } else {
    Serial.println("Error sending data: " + String(httpResponseCode));
  }
  
  http.end();
}

void sendSOSSignal() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected for SOS!");
    return;
  }
  
  Serial.println("SENDING SOS SIGNAL!");
  
  // Get current GPS location
  GPSData gpsData = readGPS();
  
  // Create SOS JSON payload
  StaticJsonDocument<512> sosDoc;
  sosDoc["timestamp"] = millis();
  sosDoc["user_id"] = userID;
  sosDoc["emergency_type"] = "panic_button";
  sosDoc["priority"] = "critical";
  
  if (gpsData.valid) {
    sosDoc["location"]["latitude"] = gpsData.latitude;
    sosDoc["location"]["longitude"] = gpsData.longitude;
  }
  
  sosDoc["device_info"]["battery_level"] = 85;
  sosDoc["device_info"]["signal_strength"] = WiFi.RSSI();
  
  String sosJsonString;
  serializeJson(sosDoc, sosJsonString);
  
  // Send SOS request
  HTTPClient http;
  http.begin(sosEndpoint);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST(sosJsonString);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("SOS sent successfully: " + response);
  } else {
    Serial.println("SOS send failed: " + String(httpResponseCode));
  }
  
  http.end();
}

GPSData readGPS() {
  GPSData gps;
  gps.valid = false;
  
  // Read GPS data from NEOM module
  // This is a simplified version - you might need to parse NMEA sentences
  if (gpsSerial.available()) {
    String gpsString = gpsSerial.readString();
    
    // Parse GPS data (simplified - you should implement proper NMEA parsing)
    if (gpsString.indexOf("$GPGGA") != -1) {
      // Extract latitude and longitude
      // This is a placeholder - implement actual NMEA parsing
      gps.latitude = 40.7128;  // Example coordinates (New York)
      gps.longitude = -74.0060;
      gps.valid = true;
    }
  }
  
  return gps;
}

String captureImage() {
  // Simplified image capture
  // In reality, you'd need to interface with the camera module
  // This returns a placeholder or base64 encoded image data
  Serial.println("Capturing image...");
  
  // Placeholder implementation
  // You would implement actual camera capture here
  return "base64_encoded_image_data_placeholder";
}

String recordAudio() {
  // Simplified audio recording
  Serial.println("Recording audio...");
  
  // Read from microphone (simplified)
  // You would implement actual audio recording here
  int audioSample = analogRead(MIC_PIN);
  
  return "base64_encoded_audio_data_placeholder";
}

// Additional utility functions for ESP32-specific features
void setupCamera() {
  // Camera initialization code for ESP32-CAM
  // This would include camera configuration
}

void setupMicrophone() {
  // Microphone setup code
  // Configure ADC for audio input
}