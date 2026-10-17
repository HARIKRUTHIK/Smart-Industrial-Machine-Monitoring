// =============================================================================
// Project: Smart Industrial Machine Monitoring & Predictive Maintenance
// Target: ESP32 + MPU6050 + DS18B20 + DHT22
// Feature: On-Device FFT Vibration Analysis & Edge Feature Extraction
// =============================================================================

#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <arduinoFFT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

// -----------------------------------------------------------------------------
// Pin Configuration
// -----------------------------------------------------------------------------
#define ONE_WIRE_BUS      4     // DS18B20 data pin on GPIO 4
#define DHTPIN            15    // DHT22 data pin on GPIO 15
#define DHTTYPE           DHT22

// FFT Parameters
#define SAMPLES           128   // Must be a power of 2
#define SAMPLING_FREQ     1000  // Hz, Nyquist limit = 500 Hz

double vReal[SAMPLES];
double vImag[SAMPLES];
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQ);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
DHT dht(DHTPIN, DHTTYPE);

const char* SERVER_URL = "http://192.168.1.100:8000/api/telemetry";

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22); // I2C SDA=21, SCL=22 for MPU6050

    ds18b20.begin();
    dht.begin();

    // MPU6050 Wake-Up
    Wire.beginTransmission(0x68);
    Wire.write(0x6B); // PWR_MGMT_1
    Wire.write(0);    // Clear sleep bit
    Wire.endTransmission(true);

    Serial.println("[OK] Industrial Monitoring Edge Node Initialized.");
}

void loop() {
    unsigned long microseconds = 1000000 / SAMPLING_FREQ;

    // 1. Acquire Vibration Data Buffer
    for (int i = 0; i < SAMPLES; i++) {
        unsigned long start = micros();

        Wire.beginTransmission(0x68);
        Wire.write(0x3B); // Accel X register
        Wire.endTransmission(false);
        Wire.requestFrom(0x68, 6, true);

        int16_t ax = Wire.read() << 8 | Wire.read();
        int16_t ay = Wire.read() << 8 | Wire.read();
        int16_t az = Wire.read() << 8 | Wire.read();

        // Acceleration in g's (Z-axis primary vibration axis)
        vReal[i] = (double)az / 16384.0;
        vImag[i] = 0.0;

        while (micros() - start < microseconds) { /* wait */ }
    }

    // 2. Perform On-Device FFT (Windowing -> FFT -> Magnitude)
    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();

    // 3. Extract Dominant Frequency & Peak Magnitude
    double dominantFreq = 0.0;
    double maxMagnitude = 0.0;
    for (int i = 2; i < (SAMPLES / 2); i++) { // Skip DC component
        if (vReal[i] > maxMagnitude) {
            maxMagnitude = vReal[i];
            dominantFreq = (i * 1.0 * SAMPLING_FREQ) / SAMPLES;
        }
    }

    // 4. Sample Surface & Environmental Sensors
    ds18b20.requestTemperatures();
    float machineSurfaceTemp = ds18b20.getTempCByIndex(0);
    float ambientTemp = dht.readTemperature();
    float ambientHumidity = dht.readHumidity();

    // 5. Output Telemetry Log
    Serial.printf("[EDGE-FFT] PeakFreq=%.1f Hz | PeakMag=%.3f | SurfaceTemp=%.1f C | AmbTemp=%.1f C | Hum=%.1f %%\n",
                  dominantFreq, maxMagnitude, machineSurfaceTemp, ambientTemp, ambientHumidity);

    // Delay before next inspection cycle
    delay(2000);
}
