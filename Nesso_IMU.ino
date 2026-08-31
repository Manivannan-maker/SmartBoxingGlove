/**
  Motor Boxing Move Data Collection for Edge Impulse
  Name: Nesso_IMU.ino
  Purpose: This sketch reads 6-axis IMU data from the onboard BMI270 sensor
  of the Nesso N1 development kit. The data is formatted for Edge Impulse
  data collection and training, with real-time display visualization.
  
  @version 1.0 01/11/25
  @author Arduino Product Experience Team
*/
#include <Arduino_Nesso_N1.h>

#include <Arduino_BMI270_BMM150.h>
#include <M5GFX.h>

// Display instance
M5GFX display;
NessoBattery battery;


// Sampling parameters
const int sampleRate = 100;                             // 100 Hz
const unsigned long sampleTime = 1000 / sampleRate;     // 10ms between samples

// Data collection variables
unsigned long lastSample = 0;

void setup() {
  // Initialize USB serial communication at 115200 baud
  Serial.begin(115200);
  
  battery.begin();
  battery.enableCharge();
  for (auto startNow = millis() + 2500; !Serial && millis() < startNow; delay(500));
  
  // Initialize display
  display.begin();
  display.setRotation(1); // Landscape mode
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  display.fillScreen(TFT_BLACK);
  display.setTextSize(1.5);
  display.setTextDatum(MC_DATUM);
  display.drawString("- Initializing IMU...", display.width() / 2, display.height() / 2);
  
  // Initialize IMU
  if (!IMU.begin()) {
    Serial.println("- Failed to initialize BMI270 IMU!");
    display.fillScreen(TFT_BLACK);
    display.setTextColor(TFT_RED, TFT_BLACK);
    display.drawString("- IMU Failed!", display.width() / 2, display.height() / 2);
    while (1);
  }
  
  Serial.println("- BOXING MOVES DATA COLLECTION (BMI270)");
  Serial.println("- 6-axis sensor initialized!");
  
  // Display sensor information
  Serial.print("- Accelerometer sample rate: ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  
  // Wait for sensor stabilization
  delay(500);
  
  // Test initial reading
  float testX, testY, testZ;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(testX, testY, testZ);
    
    Serial.print("- Initial readings (g): X=");
    Serial.print(testX, 3);
    Serial.print(", Y=");
    Serial.print(testY, 3);
    Serial.print(", Z=");
    Serial.println(testZ, 3);

    Serial.println("- Sensor ready (values already in g units)");
  }
  
  Serial.println("- Streaming continuous data for Edge Impulse!");
  Serial.println("- Data format: X_accel,Y_accel,Z_accel");
  
  // Setup display for live data
  display.fillScreen(TFT_BLACK);
  display.setTextSize(2);
  display.setTextDatum(TL_DATUM);
  display.drawString("- BOXING MOVE DATA:", 5, 5);
  display.drawString("- X:", 5, 30);
  display.drawString("- Y:", 5, 45);
  display.drawString("- Z:", 5, 60);
  display.drawString("g", 100, 30);
  display.drawString("g", 100, 45);
  display.drawString("g", 100, 60);
  
  delay(1000);
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastSample >= sampleTime) {
    float xAccel, yAccel, zAccel;
    bool dataValid = false;
    
    // Read new acceleration data from the IMU
    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(xAccel, yAccel, zAccel);
      dataValid = true;
    }
    
    // Output CSV format for Edge Impulse
    if (dataValid) {
      Serial.print(xAccel, 4);
      Serial.print(",");
      Serial.print(yAccel, 4);
      Serial.print(",");
      Serial.println(zAccel, 4);
      
      // Update display with current values
      display.fillRect(45, 30, 50, 45, TFT_BLACK);
      display.setTextColor(TFT_GREEN, TFT_BLACK);
      display.setTextSize(2.5);
      display.setCursor(40, 30);
      display.printf("%+.3f", xAccel);
      display.setCursor(40, 45);
      display.printf("%+.3f", yAccel);
      display.setCursor(40, 60);
      display.printf("%+.3f", zAccel);
    }
    
    lastSample = currentTime;
  }
}
