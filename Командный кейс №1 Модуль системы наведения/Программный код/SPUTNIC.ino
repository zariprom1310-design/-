#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>
#include <AccelStepper.h>

// --- ПИНЫ ---
const int PIN_LASER = 6;
const int PIN_SERVO = 9;
#define STEPPER_PIN1 2
#define STEPPER_PIN2 4 // Порядок 2, 4, 3, 5 критичен для драйвера ULN2003
#define STEPPER_PIN3 3
#define STEPPER_PIN4 5

// --- ОБЪЕКТЫ ---
RF24 radio(7, 8); 
const byte address[][6] = {"00001", "00002"};
Servo tiltServo;
AccelStepper panStepper(AccelStepper::HALF4WIRE, STEPPER_PIN1, STEPPER_PIN2, STEPPER_PIN3, STEPPER_PIN4);

// --- ДАННЫЕ И ТАЙМЕРЫ ---
struct Telemetry { int id = 101; int pan; int tilt; int mode; } tele;
struct Command { int mode; } cmd;

unsigned long lastBlink = 0;
unsigned long lastTele = 0;
unsigned long lastStep = 0;
int scanIdx = -40;
bool lState = false;

void setup() {
  Serial.begin(9600);
  pinMode(PIN_LASER, OUTPUT);
  
  if (!radio.begin()) {
    Serial.println("RADIO ERROR: Hardware not found!");
  } else {
    radio.setDataRate(RF24_250KBPS);
    radio.setPALevel(RF24_PA_MAX);
    radio.openWritingPipe(address[1]);
    radio.openReadingPipe(1, address[0]);
    radio.startListening();
    Serial.println("Radio Initialized OK");
  }

  tiltServo.attach(PIN_SERVO);
  panStepper.setMaxSpeed(800.0);
  panStepper.setAcceleration(400.0);
  tiltServo.write(90); 
}

void loop() {
  unsigned long now = millis();

  // 1. ПРИЕМ КОМАНД
  if (radio.available()) {
    radio.read(&cmd, sizeof(cmd));
    if (cmd.mode == 9) { 
      panStepper.setCurrentPosition(0); 
    } else { 
      tele.mode = cmd.mode; 
      scanIdx = -40; 
      lastStep = now - 3000; // Начать сразу при получении режима
    }
  }

  // 2. ЛОГИКА СКАНИРОВАНИЯ (шаг раз в 3 сек по регламенту)
  if (tele.mode != 0 && now - lastStep >= 3000 && panStepper.distanceToGo() == 0) {
    lastStep = now;
    int p = 0, t = 0;
    if (tele.mode == 1) { p = scanIdx; t = 0; }
    if (tele.mode == 2) { p = 0; t = scanIdx; }
    if (tele.mode == 3) { p = scanIdx; t = scanIdx; }
    if (tele.mode == 4) { p = scanIdx; t = -scanIdx; }

    if (scanIdx > 40) { 
      tele.mode = 0; 
      digitalWrite(PIN_LASER, LOW); 
      panStepper.moveTo(0);
      tiltServo.write(90);
    } else {
      tiltServo.write(90 + t);
      panStepper.moveTo(p * (2048.0 / 360.0));
      tele.pan = p; tele.tilt = t;
      scanIdx += 10;
    }
  }

  // 3. МИГАНИЕ ЛАЗЕРОМ (200 мс)
  if (tele.mode != 0 && now - lastBlink >= 200) {
    lastBlink = now;
    lState = !lState;
    digitalWrite(PIN_LASER, lState);
  }

  // 4. ТЕЛЕМЕТРИЯ (раз в 1.5 сек)
  if (now - lastTele >= 1500) {
    lastTele = now;
    radio.stopListening();
    radio.write(&tele, sizeof(tele));
    radio.startListening();
  }

  panStepper.run(); // Постоянное обновление мотора
}