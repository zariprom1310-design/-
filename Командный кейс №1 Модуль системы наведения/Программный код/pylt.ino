#include <SPI.h>
#include <RF24.h>

#define CE 7
#define CSN 8

RF24 radio(CE, CSN);
const uint64_t ADDR = 0xC0DEC0DE11LL;

uint32_t lastSend = 0;
uint32_t txCount = 0;

void setup() {
  Serial.begin(115200);

  radio.begin();
  radio.setChannel(40);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_LOW);       // можно MIN/LOW, начни с LOW
  radio.setCRCLength(RF24_CRC_16);
  radio.setAddressWidth(5);
  radio.disableDynamicPayloads();
  radio.setPayloadSize(1);
  radio.setAutoAck(false);

  radio.openWritingPipe(ADDR);
  radio.stopListening();

  Serial.println("TX A5 SLOW READY");
}

void loop() {
  uint32_t now = millis();
  if ((uint32_t)(now - lastSend) >= 1000) {
    lastSend = now;

    byte b = 0xA5;
    radio.write(&b, 1);

    txCount++;
    Serial.print("TX #");
    Serial.println(txCount);
  }
}
