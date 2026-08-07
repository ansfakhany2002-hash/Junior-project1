/*تعريف المنافذ*/ 
const byte STEP_PIN = 2;
const byte DIR_PIN  = 3;
const byte EN_PIN   = 4;

const byte RELAY_PIN = 5;

const byte TRIG_PIN = 6;
const byte ECHO_PIN = 7;

const byte SENSOR_A_PIN = 8;
const byte SENSOR_B_PIN = 9;

const byte MOTOR_ENABLE_LEVEL  = LOW;
const byte MOTOR_DISABLE_LEVEL = HIGH;

// Relay Active LOW
const byte RELAY_ON_LEVEL  = LOW;
const byte RELAY_OFF_LEVEL = HIGH;

/*ثوابت مهمة*/
const unsigned long STEP_INTERVAL_US = 3000;/*سرعة المحرك 3ms بين كل خطوة*/

const unsigned long PRE_FILL_PAUSE_MS = 1000;/* توقف ثانية قبل الفحص*/ 


const unsigned long PUMP_TIME_MS = 1000;/*مدة الضخ*/


const unsigned long AFTER_FILL_PAUSE_MS = 1000;/*توقف ثانية بعد الضخ*/

const unsigned long END_SENSOR_PAUSE_MS = 1000;/*توقف ثانية عند بداية ونهاية الشوط*/


long STEPS_FROM_A_TO_PUMP = 235; /*خطوات من A للمضخة*/


long STEPS_FROM_B_TO_PUMP = 230;/*خطوات منB للمضخة*/


const float ULTRASONIC_DETECT_CM = 35.0;/*مسافة اكتشاف العبوة*/ 

const bool INVERT_MOTOR_DIRECTION = false;

enum Direction {
  TOWARD_B,
  TOWARD_A
};

enum State {
  WAIT_START,/*.المضخة والمحرك واقفين ناطرين الضغط على أحد حساسات البداية*/
  /*اذا كبست a بروح عند b والعكس بالعكس*/
  MOVING_TO_PUMP,/*بتتجه القطعة لعند حساس المسافة خطوة بخطوة*/
  PRE_FILL_PAUSE,/*توقف قبل الضخ*/
  PUMPING,/*الضخ*/
  AFTER_FILL_PAUSE,
  MOVING_TO_END,
  PAUSE_AT_END_SENSOR
};

Direction currentDirection = TOWARD_B;
Direction nextDirection = TOWARD_B;

State currentState = WAIT_START;

unsigned long lastStepMicros = 0;
unsigned long stateStartMillis = 0;

long stepCounter = 0;
bool bottleDetectedAtPump = false;

void enableMotor() {
  digitalWrite(EN_PIN, MOTOR_ENABLE_LEVEL);
}

void disableMotor() {
  digitalWrite(EN_PIN, MOTOR_DISABLE_LEVEL);
}

void relayOn() {
  digitalWrite(RELAY_PIN, RELAY_ON_LEVEL);
}

void relayOff() {
  digitalWrite(RELAY_PIN, RELAY_OFF_LEVEL);
}

void setDirection(Direction dir) {
  currentDirection = dir;

  bool dirLevel = (dir == TOWARD_B) ? HIGH : LOW;

  if (INVERT_MOTOR_DIRECTION) {
    dirLevel = !dirLevel;
  }

  digitalWrite(DIR_PIN, dirLevel);

  Serial.print("Direction: ");
  Serial.println(dir == TOWARD_B ? "TOWARD_B" : "TOWARD_A");

  delay(200);
}

bool stepMotor() {
  unsigned long nowMicros = micros();

  if (nowMicros - lastStepMicros >= STEP_INTERVAL_US) {
    lastStepMicros = nowMicros;

    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(4);
    digitalWrite(STEP_PIN, LOW);

    return true;
  }

  return false;
}

bool sensorAPressed() {
  return digitalRead(SENSOR_A_PIN) == LOW;
}

bool sensorBPressed() {
  return digitalRead(SENSOR_B_PIN) == LOW;
}

float readUltrasonicCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 20000);

  if (duration == 0) {
    return -1;
  }

  float distanceCm = duration / 58.0;

  if (distanceCm < 2.0 || distanceCm > 100.0) {
    return -1;
  }

  return distanceCm;
}

bool ultrasonicBottleDetected() {
  byte goodReadings = 0;

  for (byte i = 0; i < 5; i++) {
    float distance = readUltrasonicCm();

    Serial.print("Ultrasonic read: ");
    Serial.println(distance);

    if (distance > 0 && distance <= ULTRASONIC_DETECT_CM) {
      goodReadings++;
    }

    delay(30);
  }

  return goodReadings >= 3;
}

long targetStepsToPump() {
  if (currentDirection == TOWARD_B) {
    return STEPS_FROM_A_TO_PUMP;
  } else {
    return STEPS_FROM_B_TO_PUMP;
  }
}

void startTrip(Direction dir) {
  relayOff();
  enableMotor();

  setDirection(dir);

  stepCounter = 0;
  bottleDetectedAtPump = false;

  currentState = MOVING_TO_PUMP;

  Serial.println("Trip started. Moving to pump position...");
}

void startPreFillPause() {
  disableMotor();
  relayOff();

  currentState = PRE_FILL_PAUSE;
  stateStartMillis = millis();

  Serial.println("Reached pump position.");
  Serial.println("Motor stopped before ultrasonic check.");
}

void startPumping() {
  relayOn();

  currentState = PUMPING;
  stateStartMillis = millis();

  Serial.println("Bottle detected. Pump ON for 1 second.");
}

void skipPumpingAndContinue() {
  relayOff();
  enableMotor();

  currentState = MOVING_TO_END;

  Serial.println("No bottle detected. Pump skipped.");
  Serial.println("Continuing to end sensor...");
}

void finishPumping() {
  relayOff();

  currentState = AFTER_FILL_PAUSE;
  stateStartMillis = millis();

  Serial.println("Pump OFF.");
  Serial.println("Waiting for liquid to settle.");
}

void continueToEnd() {
  relayOff();
  enableMotor();

  currentState = MOVING_TO_END;

  Serial.println("Continuing to end sensor...");
}

void startEndSensorPause(Direction directionAfterPause) {
  relayOff();
  disableMotor();

  nextDirection = directionAfterPause;
  currentState = PAUSE_AT_END_SENSOR;
  stateStartMillis = millis();

  Serial.println("End sensor touched.");
  Serial.println("Motor stopped for 1 second before reversing direction.");
}

void finishEndSensorPause() {
  Serial.println("End pause finished.");
  Serial.println("Starting reverse trip...");

  startTrip(nextDirection);
}

void setup() {
  Serial.begin(9600);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, RELAY_OFF_LEVEL);
  pinMode(RELAY_PIN, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(SENSOR_A_PIN, INPUT_PULLUP);
  pinMode(SENSOR_B_PIN, INPUT_PULLUP);

  digitalWrite(STEP_PIN, LOW);
  digitalWrite(TRIG_PIN, LOW);

  relayOff();
  disableMotor();

  Serial.println("System ready.");
  Serial.println("Place bottle at Sensor A or Sensor B.");
}

void loop() {
  switch (currentState) {

    case WAIT_START:
      relayOff();
      disableMotor();

      if (sensorAPressed()) {
        Serial.println("Sensor A touched. Moving toward B.");
        startTrip(TOWARD_B);
      }
      else if (sensorBPressed()) {
        Serial.println("Sensor B touched. Moving toward A.");
        startTrip(TOWARD_A);
      }
      break;

    case MOVING_TO_PUMP:
      enableMotor();

      if (stepMotor()) {
        stepCounter++;
      }

      if (stepCounter >= targetStepsToPump()) {
        startPreFillPause();
      }

      break;

    case PRE_FILL_PAUSE:
      if (millis() - stateStartMillis >= PRE_FILL_PAUSE_MS) {
        bottleDetectedAtPump = ultrasonicBottleDetected();

        if (bottleDetectedAtPump) {
          startPumping();
        } else {
          skipPumpingAndContinue();
        }
      }
      break;

    case PUMPING:
      if (millis() - stateStartMillis >= PUMP_TIME_MS) {
        finishPumping();
      }
      break;

    case AFTER_FILL_PAUSE:
      if (millis() - stateStartMillis >= AFTER_FILL_PAUSE_MS) {
        continueToEnd();
      }
      break;

    case MOVING_TO_END:
      enableMotor();
      stepMotor();

      if (currentDirection == TOWARD_B && sensorBPressed()) {
        Serial.println("Sensor B touched. Preparing to reverse toward A.");
        startEndSensorPause(TOWARD_A);
      }
      else if (currentDirection == TOWARD_A && sensorAPressed()) {
        Serial.println("Sensor A touched. Preparing to reverse toward B.");
        startEndSensorPause(TOWARD_B);
      }

      break;

    case PAUSE_AT_END_SENSOR:
      relayOff();
      disableMotor();

      if (millis() - stateStartMillis >= END_SENSOR_PAUSE_MS) {
        finishEndSensorPause();
      }

      break;
  }
}
