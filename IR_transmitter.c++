#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Voltas.h> // Include for Voltas Protocol

/* Wi-Fi credentials */
const char* ssid = "remote";
const char* password = "12345678";

/* WebSocket server */
WebSocketsServer webSocket = WebSocketsServer(81);

/* IR Remote Configuration */
const uint16_t kIrLedPin = D2; // GPIO4 - Connect to IR LED driver circuit
IRsend irsend(kIrLedPin);      // IRsend object

// POWER command:
// Odd press
uint8_t voltasStatePowerOdd[kVoltasStateLength]  = {0x33, 0x88, 0x88, 0x1D, 0x3B, 0x3B, 0x3B, 0x11, 0x20, 0xBD};
// Even press
uint8_t voltasStatePowerEven[kVoltasStateLength] = {0x33, 0x88, 0x08, 0x1D, 0x3B, 0x3B, 0x3B, 0x11, 0x20, 0x3D};

// UP command:
uint8_t voltasStateTempUp[kVoltasStateLength]    = {0x33, 0x88, 0x88, 0x1B, 0x3B, 0x3B, 0x3B, 0x11, 0x20, 0xBF};

// DOWN command:
uint8_t voltasStateTempDown[kVoltasStateLength]  = {0x33, 0x88, 0x88, 0x17, 0x3B, 0x3B, 0x3B, 0x11, 0x20, 0xC3};

// MODE command:
// Odd press
uint8_t voltasStateModeOdd[kVoltasStateLength]   = {0x33, 0x84, 0x88, 0x18, 0x3B, 0x3B, 0x3B, 0x11, 0x20, 0xC6};
// Even press
uint8_t voltasStateModeEven[kVoltasStateLength]  = {0x33, 0x88, 0x88, 0x1B, 0x3B, 0x3B, 0x3B, 0x11, 0x20, 0xBF};


/* Transmission Variables */
unsigned long power_count = 0;
unsigned long mode_count = 0;


void setup() {
  Serial.begin(115200);
  Serial.println("\n\nIR Transmitter Setup - Voltas Protocol Mode (Exact States)");

  /* Setup Wi-Fi */
  WiFi.mode(WIFI_AP); // Explicitly set AP mode
  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  /* Setup WebSocket */
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started on port 81");

  /* Setup IR */
  irsend.begin();
  Serial.print("IR LED Pin (GPIO): "); Serial.println(kIrLedPin); // D2 is GPIO4
  Serial.println("IR Transmitter Initialized for VOLTAS.");
  pinMode(kIrLedPin, OUTPUT);
  digitalWrite(kIrLedPin, LOW); // Ensure LED is off initially
  Serial.print("Using Voltas State Length (bytes): "); Serial.println(kVoltasStateLength);
}

void loop() {
  webSocket.loop();
}

// --- Helper Function to Send Voltas Code ---
// Uses the library-defined kVoltasStateLength constant
void sendVoltasCode(uint8_t code[]) {
  Serial.print("Sending Voltas IR Code (State Bytes): ");
  for(int i=0; i<kVoltasStateLength; i++) {
      Serial.printf("%02X ", code[i]);
  }
  Serial.println();

  // Call sendVoltas with the state byte array and the library's state length constant
  irsend.sendVoltas(code, kVoltasStateLength);
}


/***************************************** WebSocket Event Handler ******************************************/
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_TEXT) {
    String command = String((char*)payload);
    Serial.println("Command received: " + command);

    if (command == "POWER") {
      power_count++;
      if (power_count % 2 == 1) { // Odd press -> Use Power ON state
        sendVoltasCode(voltasStatePowerOdd);
      } else { // Even press -> Use Power OFF state
        sendVoltasCode(voltasStatePowerEven);
      }
    } else if (command == "UP") {
      sendVoltasCode(voltasStateTempUp);
    } else if (command == "DOWN") {
      sendVoltasCode(voltasStateTempDown);
    } else if (command == "MODE") {
      mode_count++;
      if (mode_count % 2 == 1) { // Odd press -> Use Mode 1 state
        sendVoltasCode(voltasStateModeOdd);
      } else { // Even press -> Use Mode 2 state
        sendVoltasCode(voltasStateModeEven);
      }
    } else {
      Serial.println("Unknown command");
    }
    delay(100);
  } else if (type == WStype_CONNECTED) {
      Serial.printf("[%u] WebSocket client connected from: %s\n", num, webSocket.remoteIP(num).toString().c_str());
  } else if (type == WStype_DISCONNECTED) {
      Serial.printf("[%u] WebSocket client disconnected\n", num);
  }
}
