#include <Adafruit_Sensor.h>
#include <Adafruit_TMP117.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Wire.h>
#include <math.h>

namespace {

constexpr uint8_t kDs18b20DataPin = 2;
constexpr uint8_t kHeaterGatePin = 9;
constexpr uint8_t kTmp117Address = 0x48;
constexpr uint32_t kSampleIntervalMs = 1000;
constexpr uint32_t kI2cTimeoutUs = 25000;
constexpr float kDs18b20MinimumC = -55.0F;
constexpr float kDs18b20MaximumC = 125.0F;
constexpr float kTmp117MinimumC = -55.0F;
constexpr float kTmp117MaximumC = 150.0F;
constexpr float kMaximumSensorDifferenceC = 1.0F;

// Temporary LED-only profile. Change this to false to restore the final
// empty-enclosure temperature thresholds below.
constexpr bool kBenchTestProfileEnabled = true;
constexpr float kHeaterOnBelowC =
    kBenchTestProfileEnabled ? 25.8F : 34.8F;
constexpr float kHeaterOffAboveC =
    kBenchTestProfileEnabled ? 26.2F : 35.2F;
constexpr float kHighTemperatureWarningC =
    kBenchTestProfileEnabled ? 27.0F : 36.0F;
constexpr float kHighTemperatureShutdownC =
    kBenchTestProfileEnabled ? 28.0F : 37.0F;
constexpr float kShutdownRestartBelowC =
    kBenchTestProfileEnabled ? 27.0F : 36.0F;

struct SensorReading {
  float temperatureC;
  bool valid;
  const __FlashStringHelper *status;
};

OneWire oneWire(kDs18b20DataPin);
DallasTemperature ds18b20(&oneWire);
Adafruit_TMP117 tmp117;
bool tmp117Initialized = false;
uint32_t lastSampleMs = 0;
bool heaterEnabled = false;
bool highTemperatureShutdownLatched = false;

bool intervalElapsed(uint32_t nowMs, uint32_t previousMs,
                     uint32_t intervalMs) {
  return static_cast<uint32_t>(nowMs - previousMs) >= intervalMs;
}

bool temperatureIsPlausible(float temperatureC, float minimumC,
                            float maximumC) {
  return isfinite(temperatureC) && temperatureC >= minimumC &&
         temperatureC <= maximumC;
}

bool tmp117Responds() {
  Wire.beginTransmission(kTmp117Address);
  return Wire.endTransmission() == 0;
}

SensorReading readDs18b20() {
  ds18b20.requestTemperatures();
  const float temperatureC = ds18b20.getTempCByIndex(0);

  if (ds18b20.getDeviceCount() == 0 ||
      temperatureC == DEVICE_DISCONNECTED_C) {
    return {0.0F, false, F("DS18B20_NOT_FOUND")};
  }

  if (!temperatureIsPlausible(temperatureC, kDs18b20MinimumC,
                              kDs18b20MaximumC)) {
    return {0.0F, false, F("DS18B20_INVALID")};
  }

  return {temperatureC, true, F("OK")};
}

SensorReading readTmp117() {
  if (!tmp117Initialized) {
    if (!tmp117Responds()) {
      return {0.0F, false, F("TMP117_NOT_FOUND")};
    }

    tmp117Initialized = tmp117.begin(kTmp117Address, &Wire);
    if (!tmp117Initialized) {
      return {0.0F, false, F("TMP117_INIT_FAILED")};
    }
  }

  if (!tmp117Responds()) {
    return {0.0F, false, F("TMP117_DISCONNECTED")};
  }

  sensors_event_t event;
  if (!tmp117.getEvent(&event) ||
      !temperatureIsPlausible(event.temperature, kTmp117MinimumC,
                              kTmp117MaximumC)) {
    return {0.0F, false, F("TMP117_INVALID")};
  }

  return {event.temperature, true, F("OK")};
}

void setHeaterEnabled(bool enabled) {
  heaterEnabled = enabled;
  digitalWrite(kHeaterGatePin, heaterEnabled ? HIGH : LOW);
}

void updateHighTemperatureShutdownLatch(float primaryTemperatureC,
                                        float crossCheckTemperatureC) {
  const float highestTemperatureC =
      max(primaryTemperatureC, crossCheckTemperatureC);

  if (highestTemperatureC >= kHighTemperatureShutdownC) {
    highTemperatureShutdownLatched = true;
  } else if (highTemperatureShutdownLatched &&
             primaryTemperatureC < kShutdownRestartBelowC &&
             crossCheckTemperatureC < kShutdownRestartBelowC) {
    highTemperatureShutdownLatched = false;
  }
}

void updateTemperatureControl(float primaryTemperatureC) {
  if (highTemperatureShutdownLatched) {
    setHeaterEnabled(false);
  } else if (primaryTemperatureC <= kHeaterOnBelowC) {
    setHeaterEnabled(true);
  } else if (primaryTemperatureC >= kHeaterOffAboveC) {
    setHeaterEnabled(false);
  }
}

void printTemperature(const SensorReading &reading) {
  if (reading.valid) {
    Serial.print(reading.temperatureC, 3);
  }
}

void printSample(uint32_t nowMs, const SensorReading &primary,
                 const SensorReading &crossCheck, float differenceC,
                 const __FlashStringHelper *temperatureState) {
  Serial.print(nowMs);
  Serial.print(',');
  printTemperature(primary);
  Serial.print(',');
  printTemperature(crossCheck);
  Serial.print(',');
  if (primary.valid && crossCheck.valid) {
    Serial.print(differenceC, 3);
  }
  Serial.print(',');
  Serial.print(primary.status);
  Serial.print(',');
  Serial.print(crossCheck.status);
  Serial.print(',');
  Serial.print(heaterEnabled ? F("ON,") : F("OFF,"));
  Serial.println(temperatureState);
}

void sampleAndReport(uint32_t nowMs) {
  const SensorReading primary = readDs18b20();
  const SensorReading crossCheck = readTmp117();

  if (!primary.valid || !crossCheck.valid) {
    setHeaterEnabled(false);
    printSample(nowMs, primary, crossCheck, 0.0F, F("SENSOR_FAULT"));
    return;
  }

  const float differenceC =
      fabs(primary.temperatureC - crossCheck.temperatureC);
  updateHighTemperatureShutdownLatch(primary.temperatureC,
                                     crossCheck.temperatureC);

  if (differenceC > kMaximumSensorDifferenceC) {
    setHeaterEnabled(false);
    printSample(nowMs, primary, crossCheck, differenceC,
                highTemperatureShutdownLatched ? F("SHUTDOWN_LATCHED")
                                               : F("SENSOR_DISAGREEMENT"));
    return;
  }

  updateTemperatureControl(primary.temperatureC);

  if (highTemperatureShutdownLatched) {
    printSample(nowMs, primary, crossCheck, differenceC,
                F("SHUTDOWN_LATCHED"));
  } else if (max(primary.temperatureC, crossCheck.temperatureC) >=
             kHighTemperatureWarningC) {
    printSample(nowMs, primary, crossCheck, differenceC, F("HIGH_WARNING"));
  } else {
    printSample(nowMs, primary, crossCheck, differenceC, F("NORMAL"));
  }
}

}  // namespace

void setup() {
  // Load a safe LOW value before enabling the MOSFET gate output.
  digitalWrite(kHeaterGatePin, LOW);
  pinMode(kHeaterGatePin, OUTPUT);

  Serial.begin(115200);

  ds18b20.begin();
  Wire.begin();
  Wire.setWireTimeout(kI2cTimeoutUs, true);

  Serial.println(kBenchTestProfileEnabled ? F("profile=BENCH_26C")
                                          : F("profile=ENCLOSURE_35C"));
  Serial.println(F("elapsed_ms,ds18b20_c,tmp117_c,difference_c,"
                   "primary_status,crosscheck_status,heater_state,"
                   "temperature_state"));

  const uint32_t nowMs = millis();
  lastSampleMs = nowMs - kSampleIntervalMs;
}

void loop() {
  const uint32_t nowMs = millis();
  if (intervalElapsed(nowMs, lastSampleMs, kSampleIntervalMs)) {
    lastSampleMs = nowMs;
    sampleAndReport(nowMs);
  }
}
