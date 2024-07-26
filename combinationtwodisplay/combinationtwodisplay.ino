#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h" // MAX3010x library
#include "heartRate.h" // Heart rate calculating algorithm
#include "Arduino_BMI270_BMM150.h"
#include <TinyGPS++.h>

// MAX3010x and heart rate variables
MAX30105 particleSensor;
const byte RATE_SIZE = 4; // Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; // Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; // Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define OLED1_ADDR 0x3C
#define OLED2_ADDR 0x3D

// Step counter parameters
#define STEP_THRESHOLD 1.2
int stepCount = 0;

// Create a TinyGPS++ object
TinyGPSPlus gps;

// Variables to store previous latitude and longitude
double previousLat = 0.0;
double previousLon = 0.0;
double totalDistance = 0.0;

// Timer variables
unsigned long previousMillis = 0;
const long interval = 3000; // 3 seconds
int displayMode = 0; // 0: Heart rate, 1: Step counter, 2: GPS data

void setup() {
  // Initialize Serial ports
  Serial.begin(9600);
  while (!Serial);

  // Initialize MAX3010x sensor
  particleSensor.begin(Wire, 0x3C); // Use default I2C port, 400kHz speed
  particleSensor.setup(); // Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); // Turn Red LED to low to indicate sensor is running

  // Initialize IMU sensor
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // Initialize OLED displays
  if (!display1.begin(SSD1306_SWITCHCAPVCC, OLED1_ADDR)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display1.display();
  delay(2000); // Pause for 2 seconds
  display1.clearDisplay();

  if (!display2.begin(SSD1306_SWITCHCAPVCC, OLED2_ADDR)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display2.display();
  delay(2000); // Pause for 2 seconds
  display2.clearDisplay();

  // Initialize GPS serial port
  Serial1.begin(9600); // Use Serial1 for GPS

  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.println("Acceleration in G's");
  Serial.println("X\tY\tZ");

  Serial.println("GPS Distance Calculation in Meters");
}

void loop() {
  // Update the heart rate, step count, and GPS data
  measureStepCount();
  calculateDistanceFromGPS();
  updateHeartRate();

  // Update the second display every 3 seconds
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    displayMode = (displayMode + 1) % 3; // Cycle through display modes on display2
    updateDisplay2();
  }

  // Update the first display continuously
  updateDisplay1();
}

void measureStepCount() {
  float x, y, z;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);
    if (sqrt(x * x + y * y + z * z) > STEP_THRESHOLD) {
      stepCount++;
      delay(300); // Simple debounce delay
    }
  }
}

void calculateDistanceFromGPS() {
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
    if (gps.location.isUpdated()) {
      double currentLat = gps.location.lat();
      double currentLon = gps.location.lng();
      if (previousLat != 0.0 && previousLon != 0.0) {
        double distance = calculateDistance(previousLat, previousLon, currentLat, currentLon);
        totalDistance += distance;
        Serial.print("Distance: ");
        Serial.print(distance, 6);
        Serial.print(" m, Total Distance: ");
        Serial.print(totalDistance, 6);
        Serial.println(" m");
      }
      previousLat = currentLat;
      previousLon = currentLon;
    }
  }
}

double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
  lat1 = radians(lat1);
  lon1 = radians(lon1);
  lat2 = radians(lat2);
  lon2 = radians(lon2);
  double dlat = lat2 - lat1;
  double dlon = lon2 - lon1;
  double a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1) * cos(lat2) * sin(dlon / 2) * sin(dlon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  double radius = 6371000.0; // Earth's radius in meters
  double distance = radius * c;
  return distance;
}

void updateHeartRate() {
  long irValue = particleSensor.getIR(); // Reading the IR value to know if there's a finger on the sensor or not
  if (irValue > 7000) { // If a finger is detected
    if (checkForBeat(irValue) == true) { // If a heart beat is detected
      long delta = millis() - lastBeat; // Measure duration between two beats
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0); // Calculating the BPM
      if (beatsPerMinute < 255 && beatsPerMinute > 20) { // To calculate the average we store some values (4) then do some math to calculate the average
        rates[rateSpot++] = (byte)beatsPerMinute; // Store this reading in the array
        rateSpot %= RATE_SIZE; // Wrap variable
        // Take average of readings
        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++) beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }
  } else { // If no finger is detected it informs the user and puts the average BPM to 0 or it will be stored for the next measure
    beatAvg = 0;
  }
}

void updateDisplay1() {
  display1.clearDisplay();
  display1.setTextSize(2);
  display1.setTextColor(SSD1306_WHITE);

  // Display GPS location data
  if (gps.location.isValid()) {
    display1.setCursor(0, 0);
    display1.print("Lat: ");
    display1.println(gps.location.lat(), 6);
    display1.print("Lng: ");
    display1.println(gps.location.lng(), 6);
    display1.print("Alt: ");
    display1.println(gps.altitude.meters());
  } else {
    display1.setCursor(0, 0);
    display1.println("Location: invalid");
  }

  display1.print("Date: ");
  if (gps.date.isValid()) {
    display1.print(gps.date.month());
    display1.print("/");
    display1.print(gps.date.day());
    display1.print("/");
    display1.println(gps.date.year());
  } else {
    display1.println("Not Available");
  }

  display1.print("Time: ");
  if (gps.time.isValid()) {
    int hour = gps.time.hour();
    int minute = gps.time.minute();
    hour += 5;
    minute += 30;
    if (minute >= 60) {
      minute -= 60;
      hour += 1;
    }
    if (hour >= 24) {
      hour -= 24;
    }
    if (hour < 10) display1.print("0");
    display1.print(hour);
    display1.print(":");
    if (minute < 10) display1.print("0");
    display1.print(minute);
    display1.print(":");
    if (gps.time.second() < 10) display1.print("0");
    display1.print(gps.time.second());
    display1.print(".");
    if (gps.time.centisecond() < 10) display1.print("0");
    display1.println(gps.time.centisecond());
  } else {
    display1.println("invalid");
  }
  display1.display();
}

void updateDisplay2() {
  display2.clearDisplay();
  display2.setTextSize(2);
  display2.setTextColor(SSD1306_WHITE);

  if (displayMode == 0) {
    // Display heart rate
    display2.setCursor(0, 0);
    display2.print("BPM: ");
    display2.println(beatAvg);
  } else if (displayMode == 1) {
    // Display step count
    display2.setCursor(0, 0);
    display2.print("Steps: ");
    display2.println(stepCount);
  } else if (displayMode == 2) {
    // Display total distance
    display2.setCursor(0, 0);
    display2.print("Distance: ");
    display2.print(totalDistance, 2);
    display2.println(" m");
  }
  display2.display();
}
