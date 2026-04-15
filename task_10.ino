#include <PDM.h>
#include <Arduino_APDS9960.h>
#include <Arduino_BMI270_BMM150.h>

// Threshold values
const int NOISE_THRESHOLD = 80;     
const int BRIGHT_THRESHOLD = 19;     
const float MOTION_THRESHOLD = -0.15; 
const int PROXIMITY_THRESHOLD = 220; 

// Global Variables 
short sampleBuffer[256];
volatile int samplesRead = 0;
int level = 0; 

// PDM function
void onPDMdata() {
  int bytesAvailable = PDM.available();
  PDM.read(sampleBuffer, bytesAvailable);
  samplesRead = bytesAvailable / 2;
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  PDM.onReceive(onPDMdata);
  if (!PDM.begin(1, 16000)) {
    Serial.println("Failed to start PDM!");
    while (1);
  }

  if (!APDS.begin()) {
    Serial.println("Failed to start APDS9960!");
    while (1);
  }

  if (!IMU.begin()) {
    Serial.println("Failed to start IMU!");
    while (1);
  }

  Serial.println("System Initialized");
}

void loop() {
  // Capturing Audio Data
  if (samplesRead) {
    long sum = 0;
    for (int i = 0; i < samplesRead; i++) {
      sum += abs(sampleBuffer[i]);
    }
    level = sum / samplesRead; 
    samplesRead = 0;
  }

  // Capturing Light and Proximity
  int r, g, b, brightness = 0;
  int proximity = 0;
  if (APDS.colorAvailable()) APDS.readColor(r, g, b, brightness);
  if (APDS.proximityAvailable()) proximity = APDS.readProximity();

  // Capturing IMU Motion
  float x, y, z;
  float motion = 0;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);
    motion = abs(sqrt(x*x + y*y + z*z) - 1.0);
  }

  // Classification logic
  bool isNoisy  = (level > NOISE_THRESHOLD);
  bool isBright = (brightness > BRIGHT_THRESHOLD);
  bool isMoving = (motion > MOTION_THRESHOLD);
  bool isNear   = (proximity > PROXIMITY_THRESHOLD);

  // Line 1: Raw values
  Serial.print("raw,mic="); Serial.print(level);
  Serial.print(",clear="); Serial.print(brightness);
  Serial.print(",motion="); Serial.print(motion, 3);
  Serial.print(",prox="); Serial.println(proximity);

  // Line 2: Flags 
  Serial.print("flags,sound=");
  if (isNoisy) Serial.print("1"); else Serial.print("0");
  
  Serial.print(",dark=");
  if (!isBright) Serial.print("1"); else Serial.print("0");
  
  Serial.print(",moving=");
  if (isMoving) Serial.print("1"); else Serial.print("0");
  
  Serial.print(",near=");
  if (isNear) Serial.println("1"); else Serial.println("0");

  // Line 3: State
  Serial.print("state,");
  if (!isNoisy && isBright && !isMoving && !isNear) {
    Serial.println("QUIET_BRIGHT_STEADY_FAR");
  } 
  else if (isNoisy && isBright && !isMoving && !isNear) {
    Serial.println("NOISY_BRIGHT_STEADY_FAR");
  } 
  else if (!isNoisy && !isBright && !isMoving && isNear) {
    Serial.println("QUIET_DARK_STEADY_NEAR");
  } 
  else if (isNoisy && isBright && isMoving && isNear) {
    Serial.println("NOISY_BRIGHT_MOVING_NEAR");
  } 
  else {
    Serial.println("NOT SPECIFIED SITUATION");
  }
  
  delay(500); 
}
