

/* Edge Impulse ingestion SDK
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/* Includes ---------------------------------------------------------------- */
#include <pro2_inferencing.h>
#include <Arduino_BMI270_BMM150.h> //Click here to get the library: https://www.arduino.cc/reference/en/libraries/arduino_bmi270_bmm150/
#include <ArduinoBLE.h>

/* Constant defines -------------------------------------------------------- */

/* Constant defines -------------------------------------------------------- */
#define EI_CLASSIFIER_SENSOR EI_CLASSIFIER_SENSOR_ACCELEROMETER

#define CONVERT_G_TO_MS2    9.80665f
#define MAX_ACCEPTED_RANGE  2.0f        // starting 03/2022, models are generated setting range to +-2, but this example use Arudino library which set range to +-4g. If you are using an older model, ignore this value and use 4.0f instead

/* Private variables ------------------------------------------------------- */
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal

// Global Variables

float threshold = 0.0;
float confidence = 0.8;
bool flag = false;
String Label;



//Blutooth service activating

BLEService fitness_service("e267751a-ae76-11eb-8529-0242ac130003");

BLEIntCharacteristic exercise("2A19", BLERead | BLENotify); 
BLEByteCharacteristic start("19b10012-e8f2-537e-4f6c-d104768a1214", BLERead | BLEWrite);
BLEByteCharacteristic pause("6995b940-b6f4-11eb-8529-0242ac130003", BLERead | BLEWrite);

BLEDevice central;


void setup() {
    Serial.begin(115200);
    Serial.println("Edge Impulse Inferencing Demo");

    if (!IMU.begin()) {
        Serial.println("Failed to initialize IMU!");
    } else {
        Serial.println("IMU initialized");
    }

    if (!BLE.begin()) {
        Serial.println("starting BLE failed!");
        while (1);
    }

    BLE.setLocalName("Get-Fit");
    BLE.setAdvertisedService(fitness_service);

    start.setValue(0);
    pause.setValue(0);

    fitness_service.addCharacteristic(exercise); 
    fitness_service.addCharacteristic(start);
    fitness_service.addCharacteristic(pause);

    BLE.addService(fitness_service);
    BLE.advertise();

    Serial.println("Bluetooth device active, waiting for connections...");

    while (1) {
        central = BLE.central();
        if (central) {
            Serial.print("Connected to central: ");
            Serial.println(central.address());
            digitalWrite(LED_BUILTIN, HIGH);
            break;
        }
    }
}


/**
 * @brief Return the sign of the number
 * 
 * @param number 
 * @return int 1 if positive (or 0) -1 if negative
 */
float ei_get_sign(float number) {
    return (number >= 0.0) ? 1.0 : -1.0;
}


void loop() {
    if (central.connected()) {
        start.read();
        pause.read();
    }
    if (start.value()) {
        flag = true;
        start.setValue(0);
    }
    if (pause.value()) {
        flag = false;
        pause.setValue(0);
    }
    if (flag == true) {
        float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = { 0 };

        for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += 3) {
            uint64_t next_tick = micros() + (EI_CLASSIFIER_INTERVAL_MS * 700);
            IMU.readAcceleration(buffer[ix], buffer[ix + 1], buffer[ix + 2]);

            for (int i = 0; i < 3; i++) {
                if (fabs(buffer[ix + i]) > MAX_ACCEPTED_RANGE) {
                    buffer[ix + i] = ei_get_sign(buffer[ix + i]) * MAX_ACCEPTED_RANGE;
                }
            }

            buffer[ix + 0] *= CONVERT_G_TO_MS2;
            buffer[ix + 1] *= CONVERT_G_TO_MS2;
            buffer[ix + 2] *= CONVERT_G_TO_MS2;

            delayMicroseconds(next_tick - micros());
        }

        signal_t signal;
        int err = numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
        if (err != 0) {
            Serial.println("Failed to create signal from buffer");
            return;
        }

        ei_impulse_result_t result = { 0 };
        err = run_classifier(&signal, &result, debug_nn);
        if (err != EI_IMPULSE_OK) {
            Serial.println("Failed to run classifier");
            return;
        }

        Serial.println("Predictions:");
        Serial.println("DSP: " + String(result.timing.dsp) + " ms, Classification: " + String(result.timing.classification) + " ms, Anomaly: " + String(result.timing.anomaly) + " ms");

        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            Serial.println(result.classification[ix].label + String(": ") + String(result.classification[ix].value));

            if (result.classification[ix].value > confidence) {
                Label = String(result.classification[ix].label);
                if (Label == "IDLE" && central.connected()) {
                  Serial.println("idle");
                    exercise.writeValue(0);
                } else if (Label == "SQUAT" && central.connected()) {
                    exercise.writeValue(1);
                } else if (Label == "PUSHUP" && central.connected()) {
                    exercise.writeValue(2);
                }
            }
        }
    }
}
