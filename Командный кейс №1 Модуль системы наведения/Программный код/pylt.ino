#define RF24_SPI_SPEED 500000  

#include <SPI.h>
#include <RF24.h>


#define NRF_CE   7
#define NRF_CSN  8
#define LED_PIN  6

RF24 radio(NRF_CE, NRF_CSN);
const uint8_t ADDR_CMD[6] = "CMD01"; 
const uint8_t ADDR_TLM[6] = "TLM01";  


struct Telemetry {
  int id;
  int pan;
  int tilt;
  int mode;
};


unsigned long tLed = 0;
int togglesLeft = 0;     
bool ledState = false;
bool okPulse = false;    

void ledOkPulse() {
  digitalWrite(LED_PIN, HIGH);
  okPulse = true;
  togglesLeft = 0;
  tLed = millis();
}

void ledFailBlink() {

  okPulse = false;
  togglesLeft = 6;
  ledState = false;
  digitalWrite(LED_PIN, LOW);
  tLed = millis();
}

void serviceLed() {
  unsigned long now = millis();


  if (okPulse) {
    if (now - tLed >= 200) {
      digitalWrite(LED_PIN, LOW);
      okPulse = false;
    }
    return;
  }


  if (togglesLeft > 0 && now - tLed >= 100) {
    tLed = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    togglesLeft--;
    if (togglesLeft == 0) {
      digitalWrite(LED_PIN, LOW);
    }
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(9600);

  // SPI master фикс (AVR)
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  SPI.begin();

  if (!radio.begin()) {
    Serial.println("RADIO ERROR");
    while (1);
  }

  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(100);
  radio.setRetries(5, 15);

  radio.openWritingPipe(ADDR_CMD);
  radio.openReadingPipe(1, ADDR_TLM);
  radio.startListening();

  Serial.println("REMOTE READY");
  Serial.println("Commands: 0..9");
}

void loop() {

  if (radio.available()) {
    Telemetry t;
    while (radio.available()) {
      radio.read(&t, sizeof(t));
    }

    Serial.print("DATA:");
    Serial.print(t.id);   Serial.print(",");
    Serial.print(t.pan);  Serial.print(",");
    Serial.print(t.tilt); Serial.print(",");
    Serial.println(t.mode);
  }


  if (Serial.available()) {
    char c = (char)Serial.read();
    if (c >= '0' && c <= '9') {
      radio.stopListening();
      bool ok = radio.write(&c, 1);
      radio.startListening();

      if (ok) {
        Serial.println("SENT_OK");
        ledOkPulse();
      } else {
        Serial.println("SENT_FAIL");
        ledFailBlink();
      }
    }
  }


  serviceLed();
}
