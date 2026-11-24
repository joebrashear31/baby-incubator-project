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
constexpr uint8_t kHeaterGatePin = 9;
constexpr float kHeaterOnBelowC = 34.8F;
constexpr float kHeaterOffAboveC = 35.2F;
constexpr float kHighTemperatureWarningC = 36.0F;
constexpr float kHighTemperatureShutdownC = 37.0F;
constexpr float kShutdownRestartBelowC = 36.0F;

Adafruit_TMP117 tmp117;
bool driverInitialized = false;
bool sensorConnected = false;
bool initializationFailed = false;
uint32_t lastSampleMs = 0;
uint32_t lastReconnectAttemptMs = 0;
bool heaterEnabled = false;
bool highTemperatureShutdownLatched = false;

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

void setHeaterEnabled(bool enabled) {
  heaterEnabled = enabled;
  digitalWrite(kHeaterGatePin, heaterEnabled ? HIGH : LOW);
}

void updateTemperatureControl(float temperatureC) {
  if (temperatureC >= kHighTemperatureShutdownC) {
    highTemperatureShutdownLatched = true;
  } else if (highTemperatureShutdownLatched &&
             temperatureC < kShutdownRestartBelowC) {
    highTemperatureShutdownLatched = false;
  }

  if (highTemperatureShutdownLatched) {
    setHeaterEnabled(false);
  } else if (temperatureC <= kHeaterOnBelowC) {
    setHeaterEnabled(true);
  } else if (temperatureC >= kHeaterOffAboveC) {
    setHeaterEnabled(false);
  }
}

void printUnavailableSample(uint32_t nowMs, const __FlashStringHelper *status) {
  setHeaterEnabled(false);
  Serial.print(nowMs);
  Serial.print(F(",,"));
  Serial.print(status);
  Serial.println(F(",OFF,SENSOR_FAULT"));
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

  updateTemperatureControl(event.temperature);

  Serial.print(nowMs);
  Serial.print(',');
  Serial.print(event.temperature, 3);
  Serial.print(F(",OK,"));
  Serial.print(heaterEnabled ? F("ON,") : F("OFF,"));

  if (highTemperatureShutdownLatched) {
    Serial.println(F("SHUTDOWN_LATCHED"));
  } else if (event.temperature >= kHighTemperatureWarningC) {
    Serial.println(F("HIGH_WARNING"));
  } else {
    Serial.println(F("NORMAL"));
  }
}

}  // namespace

void setup() {
  // Load a safe LOW value before enabling the MOSFET gate output.
  digitalWrite(kHeaterGatePin, LOW);
  pinMode(kHeaterGatePin, OUTPUT);

  Serial.begin(115200);

  Wire.begin();
  Wire.setWireTimeout(kI2cTimeoutUs, true);

  Serial.println(
      F("elapsed_ms,tmp117_c,sensor_status,heater_state,temperature_state"));
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
