#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN   7
#define CSN_PIN  8

RF24 radio(CE_PIN, CSN_PIN);
const byte address[][6] = {"00001", "00002"};

struct DataPackage {
  byte id;
  int currentPan;
  int currentTilt;
  byte mode;
};
DataPackage telemetry;

void setup() {
  Serial.begin(115200); // Высокая скорость для Python
  
  if (!radio.begin()) {
    while (1) {} 
  }
  
  radio.openWritingPipe(address[0]);
  radio.openReadingPipe(1, address[1]);
  radio.setPALevel(RF24_PA_MAX);
  radio.startListening();
}

void loop() {
  // 1. Из Python в Спутник
  if (Serial.available() > 0) {
    byte cmd = Serial.read();
    
    radio.stopListening();
    radio.write(&cmd, sizeof(cmd));
    radio.startListening();
  }

  // 2. Из Спутника в Python (телеметрия)
  if (radio.available()) {
    radio.read(&telemetry, sizeof(telemetry));
    
    // Отправляем данные в формате CSV для легкого чтения в Python
    Serial.print("DATA,");
    Serial.print(telemetry.mode);
    Serial.print(",");
    Serial.print(telemetry.currentPan);
    Serial.print(",");
    Serial.println(telemetry.currentTilt);
  }
}
