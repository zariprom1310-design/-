#define RF24_SPI_SPEED 500000 

#include <SPI.h>
#include <RF24.h>
#include <Servo.h>


#define NRF_CE   7
#define NRF_CSN  8


#define IN1 2
#define IN2 4
#define IN3 3
#define IN4 5

#define SERVO_PIN 9
#define LASER_PIN 6   


RF24 radio(NRF_CE, NRF_CSN);
const uint8_t ADDR_CMD[6] = "CMD01";  
const uint8_t ADDR_TLM[6] = "TLM01";  


struct Telemetry {
  int id;    
  int pan;   
  int tilt; 
  int mode;  
};


enum Mode : int { MODE_H = 0, MODE_V = 1, MODE_D1 = 2, MODE_D2 = 3 };


static inline long degToSteps(int deg);
static inline void coilsWrite(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
void serviceStepper(unsigned long now);

static inline int gridVal(int idx);
static inline int clampDeg(int v);
static inline int servoFromTilt(int tdeg);

void applyTargets();
void resetToZero();
void startFullScan(unsigned long now);
void startSingleMode(Mode m, unsigned long now);
void nextScanStep();


Servo tiltServo;


bool laserEnabled = true;  // по умолчанию ВКЛ


const long STEPS_PER_REV = 2048;
const float STEPS_PER_DEG = (float)STEPS_PER_REV / 360.0f;


const uint8_t HALFSTEP[8][4] = {
  {1,0,0,0},
  {1,1,0,0},
  {0,1,0,0},
  {0,1,1,0},
  {0,0,1,0},
  {0,0,1,1},
  {0,0,0,1},
  {1,0,0,1}
};

int stepIdx = 0;
long curSteps = 0;
long tgtSteps = 0;

const unsigned long STEPPER_PERIOD_MS = 3; 
unsigned long tStepper = 0;


bool scanning = false;
Mode currentMode = MODE_H;

int panDeg  = 0;   
int tiltDeg = 0;   

const int GRID_MIN  = -40;
const int GRID_MAX  =  40;
const int GRID_STEP =  10;
const int GRID_COUNT = ((GRID_MAX - GRID_MIN) / GRID_STEP) + 1; 

int scanIndex = 0;


const unsigned long STEP_PERIOD_MS  = 3000; 
const unsigned long TLM_PERIOD_MS   = 1500; 

unsigned long tStep  = 0;
unsigned long tTlm   = 0;

static inline long degToSteps(int deg) {
  return (long)lround(deg * STEPS_PER_DEG);
}

static inline void coilsWrite(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  digitalWrite(IN1, a);
  digitalWrite(IN2, b);
  digitalWrite(IN3, c);
  digitalWrite(IN4, d);
}

void serviceStepper(unsigned long now) {
  if (curSteps == tgtSteps) return;
  if (now - tStepper < STEPPER_PERIOD_MS) return;
  tStepper = now;

  if (curSteps < tgtSteps) {
    stepIdx = (stepIdx + 1) & 7;
    curSteps++;
  } else {
    stepIdx = (stepIdx + 7) & 7;
    curSteps--;
  }

  coilsWrite(
    HALFSTEP[stepIdx][0],
    HALFSTEP[stepIdx][1],
    HALFSTEP[stepIdx][2],
    HALFSTEP[stepIdx][3]
  );
}

static inline int gridVal(int idx) {
  return GRID_MIN + idx * GRID_STEP;
}

static inline int clampDeg(int v) {
  if (v < GRID_MIN) return GRID_MIN;
  if (v > GRID_MAX) return GRID_MAX;
  return v;
}

static inline int servoFromTilt(int tdeg) {
  int a = 90 + tdeg;  
  if (a < 0) a = 0;
  if (a > 180) a = 180;
  return a;
}

void applyTargets() {
  tgtSteps = degToSteps(panDeg);
  tiltServo.write(servoFromTilt(tiltDeg));
}

void resetToZero() {
  scanning = false;
  currentMode = MODE_H;
  scanIndex = 0;

  panDeg = 0;
  tiltDeg = 0;

  curSteps = 0;
  tgtSteps = 0;

  applyTargets();
}

void startFullScan(unsigned long now) {
  scanning = true;
  currentMode = MODE_H;
  scanIndex = 0;

  tStep = now - STEP_PERIOD_MS;
}

void startSingleMode(Mode m, unsigned long now) {
  scanning = true;
  currentMode = m;
  scanIndex = 0;
  tStep = now - STEP_PERIOD_MS;
}

void nextScanStep() {
  if (!scanning) return;

  if (currentMode == MODE_H) {
    panDeg  = 0;
    tiltDeg = gridVal(scanIndex);
  } else if (currentMode == MODE_V) {
    tiltDeg = 0;
    panDeg  = gridVal(scanIndex);
  } else if (currentMode == MODE_D1) {
    int v = gridVal(scanIndex);
    panDeg  = v;
    tiltDeg = v;
  } else { 
    int v = gridVal(scanIndex);
    panDeg  = v;
    tiltDeg = -v;
  }

  panDeg  = clampDeg(panDeg);
  tiltDeg = clampDeg(tiltDeg);
  applyTargets();

  scanIndex++;
  if (scanIndex >= GRID_COUNT) {
    scanIndex = 0;
    if (currentMode == MODE_H) currentMode = MODE_V;
    else if (currentMode == MODE_V) currentMode = MODE_D1;
    else if (currentMode == MODE_D1) currentMode = MODE_D2;
    else {
 
      resetToZero();
    }
  }
}


void setup() {
 
  pinMode(LASER_PIN, OUTPUT);
  laserEnabled = true;
  digitalWrite(LASER_PIN, HIGH);  


  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  Serial.begin(9600);


  tiltServo.attach(SERVO_PIN);
  tiltServo.write(90);


  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  SPI.begin();


  if (!radio.begin()) {
    Serial.println("RADIO ERROR");
  } else {
    radio.setDataRate(RF24_250KBPS);
    radio.setPALevel(RF24_PA_MAX);
    radio.setChannel(100);
    radio.setRetries(5, 15);

    radio.openReadingPipe(1, ADDR_CMD);
    radio.openWritingPipe(ADDR_TLM);
    radio.startListening();

    Serial.println("RADIO OK");
  }

  resetToZero();

  unsigned long now = millis();
  tStep = now;
  tTlm  = now;
}

void loop() {
  unsigned long now = millis();


  if (radio.isChipConnected() && radio.available()) {
    char cmd = 0;
    while (radio.available()) radio.read(&cmd, 1);

    if (cmd == '9') {
      resetToZero();
      Serial.println("CMD 9: RESET");
    } else if (cmd == '0') {
      startFullScan(now);
      Serial.println("CMD 0: FULL SCAN");
    } else if (cmd == '1') {
      startSingleMode(MODE_H, now);
      Serial.println("CMD 1: H");
    } else if (cmd == '2') {
      startSingleMode(MODE_V, now);
      Serial.println("CMD 2: V");
    } else if (cmd == '3') {
      startSingleMode(MODE_D1, now);
      Serial.println("CMD 3: D1");
    } else if (cmd == '4') {
      startSingleMode(MODE_D2, now);
      Serial.println("CMD 4: D2");
    } else if (cmd == '5') {
      laserEnabled = false;
      digitalWrite(LASER_PIN, LOW);
      Serial.println("CMD 5: LASER OFF");
    } else if (cmd == '6') {
      laserEnabled = true;
      digitalWrite(LASER_PIN, HIGH);
      Serial.println("CMD 6: LASER ON");
    }
  }

 
  if (scanning && (now - tStep >= STEP_PERIOD_MS)) {
    tStep = now;
    nextScanStep();
  }


  serviceStepper(now);


  if (radio.isChipConnected() && (now - tTlm >= TLM_PERIOD_MS)) {
    tTlm = now;

    Telemetry tlm;
    tlm.id = 101;
    tlm.pan = panDeg;
    tlm.tilt = tiltDeg;
    tlm.mode = scanning ? (int)currentMode : -1;

    radio.stopListening();
    radio.write(&tlm, sizeof(tlm));
    radio.startListening();
  }


  digitalWrite(LASER_PIN, laserEnabled ? HIGH : LOW);
}
