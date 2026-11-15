
#include <Servo.h>

// ==========================================
// 1. LDR PIN DEFINITIONS
// ==========================================

const int PIN_LDR_TL = A0;
const int PIN_LDR_TR = A1;
const int PIN_LDR_BL = A2;
const int PIN_LDR_BR = A3;

// ==========================================
// 2. SERVO PIN DEFINITIONS
// ==========================================

const int PIN_SERVO_HORIZ = 9; 
const int PIN_SERVO_VERT  = 10;

Servo servoHoriz;
Servo servoVert;

// ==========================================
// 3. SERVO INITIAL POSITIONS
// ==========================================

int angleHoriz = 90;
int angleVert  = 90;

// ==========================================
// 4. SAFE SERVO LIMITS
// ==========================================

// Conservative limits to avoid mechanical binding.
// Increase only after checking physical clearance.

const int HORIZ_MIN = 40;
const int HORIZ_MAX = 140;

const int VERT_MIN = 40;
const int VERT_MAX = 140;

// ==========================================
// 5. TRACKING SETTINGS
// ==========================================

// Separate tolerances for each axis
const int HORIZ_TOLERANCE = 55;
const int VERT_TOLERANCE = 40;

// One-degree movement for smoother operation
const int HORIZ_STEP = 1;
const int VERT_STEP = 2;

// Movement intervals
const unsigned long HORIZ_INTERVAL = 50;
const unsigned long VERT_INTERVAL = 50;

// Serial output interval
const unsigned long TELEMETRY_INTERVAL = 500;

unsigned long lastHorizMove = 0;
unsigned long lastVertMove = 0;
unsigned long lastPrintTime = 0;

// ==========================================
// 6. SERVO DIRECTIONS
// ==========================================

// Keep horizontal direction from your previous test.
const int HORIZ_DIRECTION = 1;

// Vertical direction reversed based on your test.
const int VERT_DIRECTION = -1;

// Set false if brighter light produces lower ADC readings.
const bool LIGHT_INCREASES_ADC = true;

// ==========================================
// 7. LDR CALIBRATION OFFSETS
// ==========================================

int offsetTL = 0;
int offsetTR = 0;
int offsetBL = 0;
int offsetBR = 0;

// ==========================================
// 8. READ SENSOR AVERAGE
// ==========================================

int readAverage(int pin) {

  long total = 0;

  for (int i = 0; i < 5; i++) {
    total += analogRead(pin);
  }

  return total / 5;
}

// ==========================================
// 9. CALIBRATE LDR SENSORS
// ==========================================

void calibrateSensors() {

  long sumTL = 0;
  long sumTR = 0;
  long sumBL = 0;
  long sumBR = 0;

  const int samples = 40;

  // Allow readings to stabilize
  delay(2000);

  for (int i = 0; i < samples; i++) {

    sumTL += analogRead(PIN_LDR_TL);
    sumTR += analogRead(PIN_LDR_TR);
    sumBL += analogRead(PIN_LDR_BL);
    sumBR += analogRead(PIN_LDR_BR);

    delay(30);
  }

  int baseTL = sumTL / samples;
  int baseTR = sumTR / samples;
  int baseBL = sumBL / samples;
  int baseBR = sumBR / samples;

  int baseAverage =
    (baseTL + baseTR + baseBL + baseBR) / 4;

  offsetTL = baseAverage - baseTL;
  offsetTR = baseAverage - baseTR;
  offsetBL = baseAverage - baseBL;
  offsetBR = baseAverage - baseBR;
}

// ==========================================
// 10. HORIZONTAL SERVO TRACKING
// ==========================================

void trackHorizontal(int avgLeft, int avgRight) {

  int difference = avgLeft - avgRight;

  int previousAngle = angleHoriz;

  if (difference > HORIZ_TOLERANCE) {

    // Left side brighter
    angleHoriz -= HORIZ_STEP * HORIZ_DIRECTION;

  }

  else if (difference < -HORIZ_TOLERANCE) {

    // Right side brighter
    angleHoriz += HORIZ_STEP * HORIZ_DIRECTION;

  }

  // Apply mechanical limits
  angleHoriz = constrain(
    angleHoriz,
    HORIZ_MIN,
    HORIZ_MAX
  );

  // Command servo only when position changes
  if (angleHoriz != previousAngle) {
    servoHoriz.write(angleHoriz);
  }
}

// ==========================================
// 11. VERTICAL SERVO TRACKING
// ==========================================

void trackVertical(int avgTop, int avgBottom) {

  int difference = avgTop - avgBottom;

  int previousAngle = angleVert;

  if (difference > VERT_TOLERANCE) {

    // Top side brighter
    angleVert += VERT_STEP * VERT_DIRECTION;

  }

  else if (difference < -VERT_TOLERANCE) {

    // Bottom side brighter
    angleVert -= VERT_STEP * VERT_DIRECTION;

  }

  // Apply mechanical limits
  angleVert = constrain(
    angleVert,
    VERT_MIN,
    VERT_MAX
  );

  if (angleVert != previousAngle) {
    servoVert.write(angleVert);
  }
}

// ==========================================
// 12. SETUP
// ==========================================

void setup() {

  Serial.begin(9600);

  // Attach both servos
  servoHoriz.attach(PIN_SERVO_HORIZ);
  servoVert.attach(PIN_SERVO_VERT);

  // Start both axes at neutral
  servoHoriz.write(90);
  servoVert.write(90);

  angleHoriz = 90;
  angleVert = 90;

  // Wait for servo movement to settle
  delay(2000);

  // Calibrate LDRs after centering
  calibrateSensors();
}

// ==========================================
// 13. MAIN LOOP
// ==========================================

void loop() {

  // Read RAW LDR values
  int rawTL = readAverage(PIN_LDR_TL);
  int rawTR = readAverage(PIN_LDR_TR);
  int rawBL = readAverage(PIN_LDR_BL);
  int rawBR = readAverage(PIN_LDR_BR);

  // Apply calibration offsets
  int valTL = constrain(
    rawTL + offsetTL, 0, 1023
  );

  int valTR = constrain(
    rawTR + offsetTR, 0, 1023
  );

  int valBL = constrain(
    rawBL + offsetBL, 0, 1023
  );

  int valBR = constrain(
    rawBR + offsetBR, 0, 1023
  );

  // ========================================
  // CALCULATE LIGHT AVERAGES
  // ========================================

  int avgTop = (valTL + valTR) / 2;
  int avgBottom = (valBL + valBR) / 2;

  int avgLeft = (valTL + valBL) / 2;
  int avgRight = (valTR + valBR) / 2;

  // Reverse light polarity if necessary
  if (!LIGHT_INCREASES_ADC) {

    int temp = avgTop;
    avgTop = avgBottom;
    avgBottom = temp;

    temp = avgLeft;
    avgLeft = avgRight;
    avgRight = temp;
  }

  unsigned long currentTime = millis();

  // ========================================
  // HORIZONTAL TRACKING
  // ========================================

  if (currentTime - lastHorizMove >= HORIZ_INTERVAL) {

    lastHorizMove = currentTime;

    trackHorizontal(avgLeft, avgRight);
  }

  // ========================================
  // VERTICAL TRACKING
  // ========================================

  if (currentTime - lastVertMove >= VERT_INTERVAL) {

    lastVertMove = currentTime;

    trackVertical(avgTop, avgBottom);
  }

  // ========================================
  // SERIAL PLOTTER OUTPUT
  // ========================================

  if (currentTime - lastPrintTime >= TELEMETRY_INTERVAL) {

    lastPrintTime = currentTime;

    Serial.print("TL:");
    Serial.print(rawTL);

    Serial.print(",TR:");
    Serial.print(rawTR);

    Serial.print(",BL:");
    Serial.print(rawBL);

    Serial.print(",BR:");
    Serial.print(rawBR);

    Serial.print(",Hangle:");
    Serial.print(angleHoriz);

    Serial.print(",Vangle:");
    Serial.println(angleVert);
  }
}
