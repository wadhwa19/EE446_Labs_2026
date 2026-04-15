#include <Arduino_HS300x.h>
#include <Arduino_BMI270_BMM150.h>
#include <Arduino_APDS9960.h>

// Threshold values
const float HUMID_JUMP_THRESHOLD = 60;   
const float TEMP_RISE_THRESHOLD = 27;    
const float MAG_SHIFT_THRESHOLD = 8;   
const int LIGHT_CHANGE_THRESHOLD = 105;    


float baseHumid, baseTemp, baseMag;
int baseClear;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(1500);

  //  Sensors Initializion
  if (!HS300x.begin()) {
    Serial.println("Failed to initialize humidity/temperature sensor.");
    while (1);
  }

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU.");
    while (1);
  }

  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960 sensor.");
    while (1);
  }

  //  Capture Baselines 
  baseHumid = HS300x.readHumidity();
  baseTemp = HS300x.readTemperature();
  
  float mx, my, mz;
  while (!IMU.magneticFieldAvailable());
  IMU.readMagneticField(mx, my, mz);
  baseMag = sqrt(mx*mx + my*my + mz*mz); 

  int r, g, b;
  while (!APDS.colorAvailable());
  APDS.readColor(r, g, b, baseClear);

  Serial.println("Baselines calibrated.");
}

void loop() {
 
  
  // HS3003 Data
  float currentTemp = HS300x.readTemperature();
  float currentHumid = HS300x.readHumidity();

  // Magnetometer Data
  float mx, my, mz, currentMag = 0;
  if (IMU.magneticFieldAvailable()) {
    IMU.readMagneticField(mx, my, mz);
    currentMag = sqrt(mx*mx + my*my + mz*mz);
  }

  // Light and Color Data
  int r, g, b, currentClear = 0;
  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b, currentClear);
  }

  //  Decision logic
  
  bool humid_jump = (currentHumid - baseHumid > HUMID_JUMP_THRESHOLD);
  bool temp_rise  = (currentTemp - baseTemp > TEMP_RISE_THRESHOLD);
  bool mag_shift  = (abs(currentMag - baseMag) > MAG_SHIFT_THRESHOLD);
  bool light_or_color_change = (abs(currentClear - baseClear) > LIGHT_CHANGE_THRESHOLD);

  //Classification 
  
  String eventLabel = "BASELINE_NORMAL";
  
  
  if (humid_jump || temp_rise) {
    eventLabel = "BREATH_OR_WARM_AIR_EVENT";
  } 
  else if (mag_shift) {
    eventLabel = "MAGNETIC_DISTURBANCE_EVENT";
  } 
  else if (light_or_color_change) {
    eventLabel = "LIGHT_OR_COLOR_CHANGE_EVENT";
  }

 

  // Line 1 output- Raw Data
  Serial.print("raw,rh="); Serial.print(currentHumid);
  Serial.print(",temp="); Serial.print(currentTemp);
  Serial.print(",mag="); Serial.print(currentMag);
  Serial.print(",r="); Serial.print(r);
  Serial.print(",g="); Serial.print(g);
  Serial.print(",b="); Serial.print(b);
  Serial.print(",clear="); Serial.println(currentClear);

  // Line 2: Flags 
  Serial.print("flags,humid_jump=");
  if (humid_jump) Serial.print("1"); else Serial.print("0");
  Serial.print(",temp_rise=");
  if (temp_rise) Serial.print("1"); else Serial.print("0");
  Serial.print(",mag_shift=");
  if (mag_shift) Serial.print("1"); else Serial.print("0");
  Serial.print(",light_or_color_change=");
  if (light_or_color_change) Serial.println("1"); else Serial.println("0");

  // Line 3: Event Label
  Serial.print("event,");
  Serial.println(eventLabel);

  delay(500); 
}