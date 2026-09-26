# Smart AI Boxing Band

## Overview

The Smart AI Boxing Band is an Edge AI wearable that recognises boxing punches in real time using onboard IMU data and machine learning deployed with Edge Impulse. The device runs entirely on the Arduino Nesso N1 and provides instant feedback during training sessions without requiring cloud connectivity.

Key features include:

- Real-time punch classification
- On-device TinyML inference
- Punch counting and analytics
- Heart-rate monitoring
- Visual training intensity feedback
- Fully offline operation

---

## Problem Statement

Boxers and fitness enthusiasts often lack objective training metrics during practice sessions. Coaches typically rely on visual observation to evaluate performance, making it difficult to quantify punch frequency, movement patterns, and workout intensity.

This project aims to solve this problem by combining Edge AI and wearable sensing technology to provide real-time training analytics directly on the athlete's wrist.

---

## Hardware Used

### Arduino Nesso N1

The Arduino Nesso N1 provides:

- ESP32-C6 processor
- Built-in 6-axis IMU
- Integrated touchscreen display
- Battery-powered operation
- Wireless connectivity

### Additional Components

- Pulse Sensor
- Wrist strap
- Jumper wires

---

# Machine Learning Pipeline

The machine learning workflow was developed entirely using Edge Impulse.

## Data Acquisition

Motion data was collected using the onboard IMU while the boxing band was worn during training sessions.

The following movement classes were recorded:

| Label | Description |
|---------|-------------|
| Idle | No movement |
| JabCross | Straight punch combination |
| Hook | Circular punching motion |
| UpperCut | Upward punching motion |

Each class was captured multiple times to ensure representative training samples.

---

## Data Collection Process

1. Connect Arduino Nesso N1 to Edge Impulse Data Forwarder.
2. Stream accelerometer data to Edge Impulse Studio.
3. Perform boxing movements while wearing the device.
4. Label each recording.
5. Upload samples to the training dataset.
6. Repeat until balanced datasets are created.

---

## Impulse Design

### Processing Block

**Spectral Analysis**

The spectral feature extraction block was selected because boxing motions generate distinct frequency-domain patterns that can be used for classification.

### Learning Block

**Classification**

A neural network classifier was trained using the extracted motion features.

### Configuration

| Parameter | Value |
|------------|----------|
| Window Size | 1200 ms |
| Window Increase | 750 ms |

---

## Feature Generation

After feature extraction, the Feature Explorer showed clear separation between all boxing movement classes.

Classes formed distinct clusters for:

- Idle
- JabCross
- Hook
- UpperCut

This indicated the collected IMU data contained sufficient information for accurate classification.

---

# Model Training

The classification model was trained directly within Edge Impulse Studio.

## Training Settings

| Parameter | Value |
|-----------|---------|
| Epochs | 100 |
| Learning Rate | 0.05 |
| Classes | 4 |
| Learning Block | Neural Network Classifier |

### Training Workflow

1. Collect labelled IMU data.
2. Generate spectral features.
3. Train neural network.
4. Evaluate validation results.
5. Review confusion matrix.
6. Optimise dataset if necessary.
7. Export trained model.

---

## Training Results

### Training Accuracy

**100% Accuracy**

The neural network successfully learned the movement patterns associated with all four boxing activities.

### Classification Performance

The resulting model demonstrated clear separation between:

- Idle
- JabCross
- Hook
- UpperCut

Feature clustering and validation metrics indicated highly reliable class recognition.

---

## Testing Results

The model was tested using data that was not included in training.

### Test Accuracy

**100% Accuracy**

The model consistently recognised punching movements during real-time testing, demonstrating reliable classification performance on the device.

---

# Deployment

After validation, the Edge Impulse model was exported as an Arduino Library and integrated into the Arduino IDE.

The model performs inference directly on the Arduino Nesso N1.

Benefits include:

- No internet dependency
- Low latency predictions
- Local processing
- Reduced power consumption
- Real-time feedback

---

# Real-Time Punch Recognition

The deployed application continuously performs the following steps:

1. Read IMU sensor data.
2. Prepare data window for inference.
3. Run Edge Impulse classification.
4. Display detected punch type.
5. Update punch counters.
6. Generate training statistics.

Recognised classes include:

- Idle
- JabCross
- Hook
- UpperCut

---

# Heart Rate Monitoring

A Pulse Sensor is integrated into the wearable and operates alongside the machine learning model.

The display colour changes according to detected heart rate:

| Heart Rate | Display Colour |
|------------|----------------|
| Below 100 BPM | Green |
| 100-140 BPM | Blue |
| 140-160 BPM | Yellow |
| Above 160 BPM | Red |
| No Reading | Black |

This provides a simple visual indication of workout intensity.

---

# Why Edge Impulse?

Edge Impulse significantly accelerated development by providing:

- Data acquisition tools
- Sample labelling
- Feature extraction
- Neural network training
- Model validation
- One-click deployment

The platform enabled rapid development of a complete embedded machine learning solution without building a custom ML pipeline from scratch.

---

# Results Summary

| Metric | Value |
|----------|---------|
| Sensor | 6-Axis IMU |
| Classes | 4 |
| Processing Block | Spectral Analysis |
| Learning Block | Classification |
| Training Accuracy | 100% |
| Test Accuracy | 100% |
| Deployment Target | Arduino Nesso N1 |
| Inference | On Device |

---

# Applications

Potential use cases include:

- Boxing analytics
- Combat sports monitoring
- Fitness tracking
- Smart coaching systems
- Athlete performance analysis
- Sports technology research

---

# Future Improvements

Future enhancements may include:

- Additional punch classifications
- Punch force estimation
- Fatigue detection
- Workout history tracking
- Cloud dashboard integration
- Multi-sensor fusion
- Coach analytics platform

---

# Conclusion

The Smart AI Boxing Band demonstrates how Edge Impulse can be used to build an intelligent sports wearable capable of recognising boxing punches in real time. By combining TinyML-based motion classification with heart-rate monitoring, the system provides meaningful training insights while running entirely on embedded hardware. The project highlights the potential of Edge AI for sports performance monitoring, low-latency analytics, and next-generation wearable coaching solutions.

---

## Project Links

- Hackster Project: https://www.hackster.io/manivannan/smart-ai-boxing-band-bddc39
- Edge Impulse: https://studio.edgeimpulse.com/public/1081144/live

