#include <Adafruit_Sensor.h>
#include <Adafruit_TMP117.h>
#include <Wire.h>
#include <math.h>

namespace {

constexpr uint8_t kTmp117Address = 0x48;
constexpr uint32_t kSampleIntervalMs = 1000;
constexpr uint32_t kReconnectIntervalMs = 2000;
constexpr uint32_t kI2cTimeoutUs = 25000;
constexpr float kMinimumPlausibleTemperatureC = -55.0F;
constexpr float kMaximumPlausibleTemperatureC = 150.0F;

Adafruit_TMP117 tmp117;
bool driverInitialized = false;
bool sensorConnected = false;
bool initializationFailed = false;
uint32_t lastSampleMs = 0;
uint32_t lastReconnectAttemptMs = 0;

bool intervalElapsed(uint32_t nowMs, uint32_t previousMs,
                     uint32_t intervalMs) {
  return static_cast<uint32_t>(nowMs - previousMs) >= intervalMs;
}

bool tmp117Responds() {
  Wire.beginTransmission(kTmp117Address);
  return Wire.endTransmission() == 0;
}

bool initializeTmp117() {
  if (!tmp117Responds()) {
    return false;
  }

  driverInitialized = tmp117.begin(kTmp117Address, &Wire);
  initializationFailed = !driverInitialized;
  sensorConnected = driverInitialized;
  return driverInitialized;
}

void printUnavailableSample(uint32_t nowMs, const __FlashStringHelper *status) {
  Serial.print(nowMs);
  Serial.print(F(",,"));
  Serial.println(status);
}

void sampleAndReport(uint32_t nowMs) {
  if (initializationFailed) {
    printUnavailableSample(nowMs, F("SENSOR_INIT_FAILED"));
    return;
  }

  if (!driverInitialized) {
    printUnavailableSample(nowMs, F("SENSOR_NOT_FOUND"));
    return;
  }

  if (!tmp117Responds()) {
    sensorConnected = false;
    printUnavailableSample(nowMs, F("SENSOR_DISCONNECTED"));
    return;
  }

  sensorConnected = true;

  sensors_event_t event;
  if (!tmp117.getEvent(&event) || !isfinite(event.temperature) ||
      event.temperature < kMinimumPlausibleTemperatureC ||
      event.temperature > kMaximumPlausibleTemperatureC) {
    printUnavailableSample(nowMs, F("INVALID_READING"));
    return;
  }

  Serial.print(nowMs);
  Serial.print(',');
  Serial.print(event.temperature, 3);
  Serial.println(F(",OK"));
}

}  // namespace

void setup() {
  Serial.begin(115200);

  Wire.begin();
  Wire.setWireTimeout(kI2cTimeoutUs, true);

  Serial.println(F("elapsed_ms,tmp117_c,status"));
  initializeTmp117();

  const uint32_t nowMs = millis();
  lastSampleMs = nowMs - kSampleIntervalMs;
  lastReconnectAttemptMs = nowMs;
}

void loop() {
  const uint32_t nowMs = millis();

  if (!initializationFailed && !sensorConnected &&
      intervalElapsed(nowMs, lastReconnectAttemptMs, kReconnectIntervalMs)) {
    lastReconnectAttemptMs = nowMs;
    if (driverInitialized) {
      sensorConnected = tmp117Responds();
    } else {
      initializeTmp117();
    }
  }

  if (intervalElapsed(nowMs, lastSampleMs, kSampleIntervalMs)) {
    lastSampleMs = nowMs;
    sampleAndReport(nowMs);
  }
}
