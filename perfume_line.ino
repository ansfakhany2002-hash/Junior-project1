const int TRIG_PIN        = 9;
const int ECHO_PIN        = 10;
const int RELAY_PIN       = 7;
const int DETECT_DISTANCE = 20;
const int PUMP_DURATION   = 2000;
const int COOLDOWN_TIME   = 5000;

unsigned long lastSprayTime = 0;

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  Serial.begin(9600);
}

void loop() {
  long distance = measureDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  unsigned long currentTime = millis();

  if (distance < DETECT_DISTANCE && distance > 2) {
    if (currentTime - lastSprayTime >= COOLDOWN_TIME) {
      sprayPerfume();
      lastSprayTime = currentTime;
    }
  }

  delay(100);
}

long measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  long distance  = duration * 0.0343 / 2;
  return distance;
}

void sprayPerfume() {
  digitalWrite(RELAY_PIN, LOW);
  delay(PUMP_DURATION);
  digitalWrite(RELAY_PIN, HIGH);
}
