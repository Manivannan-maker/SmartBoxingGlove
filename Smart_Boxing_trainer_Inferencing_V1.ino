#include <Smart_Boxing_Trainer_V2_inferencing.h>

/**
  Smart boxing trainer
  Name: Smart_Boxing_trainer.ino
  Purpose: This sketch implements real-time Boxing move data analysis using
  the Nesso N1's integrated BMI270 IMU and Edge Impulse machine learning 
  model with full-screen visual feedback .
  Date : 31-08-2026
  Author - manivannan sivan
*/

// Include the Edge Impulse library (name will match your project name)
#include <Arduino_Nesso_N1.h>

#include <PulseSensorPlayground.h>  // Includes the PulseSensorPlayground Library.

//  Variables
const int PulseWire = 5;      // PulseSensor PURPLE WIRE connected to ANALOG PIN 5
int Threshold = 350;          // Determine which Signal to "count as a beat" and which to ignore.
                              // Use the "Gettting Started Project" to fine-tune Threshold Value beyond default setting.
                              // Otherwise leave the default "350" value.
int myBPM =0;
// Include libraries for Nesso's IMU and display control
#include <Arduino_BMI270_BMM150.h>
#include <M5GFX.h>

// Display instance for the 1.14" touch screen
M5GFX display;
NessoBattery battery;
PulseSensorPlayground pulseSensor;  // Creates an instance of the PulseSensorPlayground object called "pulseSensor"


// Punch counters
int idleCount = 0;
int hookCount = 0;
int jabCrossCount = 0;
int uppercutCount = 0;
String lastState = "Idle";

// Data buffers for model inference
static float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = { 0 };
static float inference_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
ei_classifier_smooth_t smooth;


// Maximum accepted acceleration range (±2g)
#define MAX_ACCEPTED_RANGE  2.0f

// Detection parameters
const float ANOMALY_THRESHOLD = 3.0f;   // Threshold for anomaly detection
const float WARNING_THRESHOLD = 1.5f;   // Warning zone threshold
const float IDLE_THRESHOLD = 0.02f;     // Vibration threshold for idle detection

// System status variables
int totalInferences = 0;
int anomalyCount = 0;
unsigned long lastInferenceTime = 0;
const unsigned long INFERENCE_INTERVAL = 500; // Inference interval in milliseconds

// Buffer management variables
bool bufferFilled = false;
int sampleCount = 0;

// Current state tracking
String currentState = "INITIALIZING";
bool currentAnomaly = false;

// Function declarations
float ei_get_sign(float number);
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr);
void runInference();
void updateFullScreenDisplay(String state, bool anomaly);
float calculateVibrationLevel();
void processResults(ei_impulse_result_t result, float vibration);

/**
  Initializes the IMU, display, and machine learning system.
  Configures the Nesso N1 for optimal performance with the
*/
void setup() {
    // Initialize serial communication at 115200 baud
    Serial.begin(115200);
    battery.begin();
  battery.enableCharge();

  // Configure the PulseSensor object, by assigning our variables to it.
  pulseSensor.analogInput(PulseWire);
  pulseSensor.setThreshold(Threshold);

  // Double-check the "pulseSensor" object was created and "began" seeing a signal.
  if (pulseSensor.begin()) {
    Serial.println("We created a pulseSensor Object !");  // This prints one time at Arduino power-up,  or on Arduino reset.
  }
    
    
    Serial.println("- Nesso N1 smart Boxing trainer");
    
    // Initialize the 1.14" touch display
    display.begin();
    display.setRotation(1);  // Set to landscape orientation
    display.fillScreen(TFT_BLACK);
    display.setTextSize(2);
    display.setTextColor(TFT_WHITE, TFT_BLACK);
    display.setTextDatum(MC_DATUM);
    display.drawString("INITIALIZING...", display.width() / 2, display.height() / 2);
    
    // Initialize BMI270 IMU sensor
    if (!IMU.begin()) {
        Serial.println("- ERROR: Failed to initialize IMU!");
        display.fillScreen(TFT_RED);
        display.setTextColor(TFT_WHITE, TFT_RED);
        display.drawString("IMU FAILED!", display.width() / 2, display.height() / 2);
        while (1);  // Halt execution on IMU failure
    }
    
    Serial.println("- BMI270 IMU initialized!");
    Serial.print("- Sample rate: ");
    Serial.print(IMU.accelerationSampleRate());
    Serial.println(" Hz");
    
    // Verify Edge Impulse model configuration
    if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3) {
        Serial.println("ERROR: EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME should be 3");
        while (1);  // Halt execution on configuration error
    }
    
    Serial.println("\n- Edge Impulse Model loaded!");
    Serial.print("- Project: ");
    Serial.println(EI_CLASSIFIER_PROJECT_NAME);
    
    Serial.println("\n- Filling buffer...");
    
    // Display starting message while buffer fills
    display.fillScreen(TFT_DARKGREY);
    display.setTextColor(TFT_WHITE, TFT_DARKGREY);
    display.drawString("STARTING...", display.width() / 2, display.height() / 2);
   ei_classifier_smooth_init(&smooth, 10, 7, 0.8, 0.3);
    
    delay(1000);
}

/**
  Main loop that continuously collects vibration data and performs
  real-time classification and anomaly detection using the embedded
  machine learning models.
*/
void loop() {
    // Calculate the next sampling tick for precise timing
    //Serial.print("EI_CLASSIFIER_INTERVAL_MS : ");
    //Serial.println(EI_CLASSIFIER_INTERVAL_MS);

 if (pulseSensor.sawStartOfBeat()) {              // Constantly test to see if "a beat happened".
     myBPM = pulseSensor.getBeatsPerMinute();   // Calls function on our pulseSensor object that returns BPM as an "int".
                                                   // "myBPM" hold this BPM value now.
    Serial.println("♥  A HeartBeat Happened ! ");  // If test is "true", print a message "a heartbeat happened".
    Serial.print("BPM: ");                         // Print phrase "BPM: "
    Serial.println(myBPM);                         // Print the value inside of myBPM.
  }

    
    uint64_t next_tick = micros() + (EI_CLASSIFIER_INTERVAL_MS * 1000);
    
    // Shift the buffer by 3 samples to create a rolling window
    numpy::roll(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, -3);
    
    // Wait for new acceleration data from the IMU
    float x, y, z;
    while (!IMU.accelerationAvailable()) {
        delayMicroseconds(10);
    }
    
    // Read acceleration values (already in g units)
    IMU.readAcceleration(x, y, z);
    
    // Store new data at the end of the buffer
    buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 3] = x;
    buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 2] = y;
    buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 1] = z;
    
    // Clip acceleration values to the maximum accepted range
    for (int i = 0; i < 3; i++) {
        float* val = &buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 3 + i];
        if (fabs(*val) > MAX_ACCEPTED_RANGE) {
            *val = ei_get_sign(*val) * MAX_ACCEPTED_RANGE;
        }
    }
    
    // Track buffer filling progress during initialization
    if (!bufferFilled) {
        sampleCount++;
        Serial.print("tick before buffer filling : ");
        Serial.println(millis());
        if (sampleCount >= EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE / 3) {
            bufferFilled = true;
            Serial.println("- Buffer filled, starting monitoring...\n");
            Serial.print("tick after  buffer filling : ");
        Serial.println(millis());
        }
    }
    
    // Maintain precise sampling rate
    uint64_t time_to_wait = next_tick - micros();
    if (time_to_wait > 10000 && time_to_wait < 1000000) {
      //Serial.println(time_to_wait);
     
        delayMicroseconds(time_to_wait);
    }
   // Serial.print(" INFERENCE_INTERVAL : ");
   // Serial.println(INFERENCE_INTERVAL);
    // Execute inference at the specified interval
    if (bufferFilled && (millis() - lastInferenceTime >= (INFERENCE_INTERVAL+250))) {
        lastInferenceTime = millis();
        runInference();

         // ✅ Reset buffer after inference
    for (int i = 0; i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; i++) {
        buffer[i] = 0.0f;   // clear all values
        bufferFilled=false;
    }
    }
}

/**
  Executes the Edge Impulse inference on collected boxing moves data.
  Processes the data through  classification .
*/
void runInference() {
    // Copy the current buffer for inference processing
    memcpy(inference_buffer, buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE * sizeof(float));
    
    
    
    // Create signal structure for Edge Impulse
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    signal.get_data = &raw_feature_get_data;
    
    // Run the Edge Impulse classifier
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
    
    if (res != EI_IMPULSE_OK) {
        Serial.printf("- ERROR: Failed to run classifier (%d)!\n", res);
        return;
    }
    
   
    // Process and display the inference results
    processResults(result, 0);
}

/**
  Processes inference results and updates the full-screen display.
  Analyzes classification confidence to determine
  the boxing moves and trigger appropriate visual feedback.
*/
void processResults(ei_impulse_result_t result, float vibration) {
    totalInferences++;
    
    // Find the classification with highest confidence
    String bestLabel = "unknown";
    float bestValue = 0;
    
    Serial.printf("- Inference #%d\n", totalInferences);
    
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
      Serial.printf((ei_classifier_inferencing_categories[ix]));
      Serial.println(result.classification[ix].value);
        if (result.classification[ix].value > bestValue) {
            bestValue = result.classification[ix].value;
            bestLabel = String(ei_classifier_inferencing_categories[ix]);
        }
    }
    
    Serial.printf("- State: %s (%.0f%% confidence)\n", bestLabel.c_str(), bestValue * 100);
    Serial.printf("- Vibration: %.4f g\n", vibration);
    
    bool isAnomaly = false;
    
#if EI_CLASSIFIER_HAS_ANOMALY
    float anomalyScore = result.anomaly;
    Serial.printf("- Anomaly score: %.3f", anomalyScore);
    
    if (anomalyScore < WARNING_THRESHOLD) {
        Serial.println(" [NORMAL]");
    } else if (anomalyScore < ANOMALY_THRESHOLD) {
        Serial.println(" [WARNING]");
    } else {
        Serial.println(" [ANOMALY!]");
        isAnomaly = true;
        anomalyCount++;
    }
#endif
    
    // Update display only when state or anomaly status changes
    if (bestLabel != currentState || isAnomaly != currentAnomaly) {
        currentState = bestLabel;
        currentAnomaly = isAnomaly;
        updateFullScreenDisplay(currentState, currentAnomaly);
    }
    
    Serial.printf("- Timing: DSP %d ms, Classification %d ms\n\n", 
                  result.timing.dsp, result.timing.classification);

// Increase counters for Jab-Cross , hook , Idle , uppercount for displaying 

                  if (bestLabel == "Idle") {
    idleCount++;
} else if (bestLabel == "Hook") {
    hookCount++;
} else if (bestLabel == "Jab_Cross") {
    jabCrossCount++;
} else if (bestLabel == "Uppercut") {
    uppercutCount++;
}

    Serial.print(" Jab_Cross :  ");
    Serial.println(jabCrossCount);

Serial.print(" Uppercut :  ");
    Serial.println(uppercutCount);

Serial.print(" Hook :  ");
    Serial.println(hookCount);

Serial.print(" Idle :  ");
    Serial.println(idleCount);


}



void updateFullScreenDisplay(String state, bool anomaly) {
    uint16_t bgColor;
    uint16_t textColor;
    String displayText;
     lastState = state;

    if (state == "Idle") {
        bgColor = TFT_BLUE;
        textColor = TFT_WHITE;
        displayText = "IDLE: " + String(idleCount);
    } else if (state == "Hook"&& lastState == "Idle") {
      //Serial.print("Last state for Hook : ");
      //Serial.println(lastState);
        bgColor = TFT_GREEN;
        textColor = TFT_BLACK;
        displayText = "HOOK: " + String(hookCount);
    } else if (state == "Jab_Cross" && lastState == "Idle") {
//Serial.print("Last state for Jab Cross : ");
      //Serial.println(lastState);
      
        bgColor = TFT_GREEN;
        textColor = TFT_BLACK;
        displayText = "JAB_CROSS: " + String(jabCrossCount);
    } else if (state == "Uppercut" && lastState == "Idle") {
    //  Serial.print("Last state for Upper cut : ");
     // Serial.println(lastState);
        bgColor = TFT_YELLOW;
        textColor = TFT_BLACK;
        displayText = "UPPERCUT: " + String(uppercutCount);
    } else {
        bgColor = TFT_DARKGREY;
        textColor = TFT_WHITE;
        displayText = "UNKNOWN";
    }

   

  // Update display with current values
  if(myBPM<100)
  {

          display.fillScreen(TFT_GREEN);

  }
  else if(myBPM>100 && myBPM<130)
  {
      display.fillScreen(TFT_BLUE);

    
  }

  else if(myBPM>130 && myBPM < 160)

  {
      display.fillScreen(TFT_YELLOW);

    
  }

  else if(myBPM>160 && myBPM <200)

  {
      display.fillScreen(TFT_RED);

    
  }

  else
  {
          display.fillScreen(TFT_BLACK);

  }
      display.setTextColor(TFT_GREEN, TFT_BLACK);
      display.setTextSize(2);
      display.setTextDatum(TL_DATUM);
      display.drawString("- BOXING MOVE DATA:", 5, 5);
  display.drawString("Jab Cross:", 5, 30);
  display.drawString("Hook:", 5, 60);
  display.drawString("Upper Cut:", 5, 90);
   display.drawString("Heart Ratet:", 5, 120);
  
  
      display.setCursor(150, 30);
      display.printf("%d", jabCrossCount);
      display.setCursor(150, 60);
      display.printf("%d", hookCount);
      display.setCursor(150, 90);
      display.printf("%d", uppercutCount);

       display.setCursor(150, 120);
      display.printf("%d", myBPM);


    
   
}

/**
  Returns the sign of a number.
  Used for clipping acceleration values to the maximum range.
*/
float ei_get_sign(float number) {
    return (number >= 0.0) ? 1.0 : -1.0;
}

/**
  Callback function for Edge Impulse library to access feature data.
  Provides the machine learning model with boxing move in the
  required format for inference processing.
*/
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, inference_buffer + offset, length * sizeof(float));
    return 0;
}
