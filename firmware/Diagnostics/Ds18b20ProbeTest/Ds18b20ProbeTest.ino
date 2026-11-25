#include <DallasTemperature.h>
#include <OneWire.h>
#include <math.h>

namespace {

constexpr uint8_t kProbeDataPin = 2;
constexpr uint8_t kHeaterGatePin = 9;
constexpr uint32_t kSampleIntervalMs = 1000;
constexpr float kMinimumPlausibleTemperatureC = -55.0F;
constexpr float kMaximumPlausibleTemperatureC = 125.0F;

OneWire oneWire(kProbeDataPin);
DallasTemperature probe(&oneWire);
uint32_t lastSampleMs = 0;

bool intervalElapsed(uint32_t nowMs, uint32_t previousMs,
                     uint32_t intervalMs) {
  return static_cast<uint32_t>(nowMs - previousMs) >= intervalMs;
}

void sampleAndReport(uint32_t nowMs) {
  probe.requestTemperatures();
  const float temperatureC = probe.getTempCByIndex(0);

  Serial.print(nowMs);
  Serial.print(',');

  if (probe.getDeviceCount() == 0 ||
      temperatureC == DEVICE_DISCONNECTED_C || !isfinite(temperatureC) ||
      temperatureC < kMinimumPlausibleTemperatureC ||
      temperatureC > kMaximumPlausibleTemperatureC) {
    Serial.println(F(",PROBE_NOT_FOUND"));
    return;
  }

  Serial.print(temperatureC, 3);
  Serial.println(F(",OK"));
}

}  // namespace

void setup() {
  // Keep the MOSFET gate off while this diagnostic is running.
  digitalWrite(kHeaterGatePin, LOW);
  pinMode(kHeaterGatePin, OUTPUT);

  Serial.begin(115200);
  probe.begin();

  Serial.println(F("elapsed_ms,ds18b20_c,sensor_status"));
  lastSampleMs = millis() - kSampleIntervalMs;
}

void loop() {
  const uint32_t nowMs = millis();
  if (intervalElapsed(nowMs, lastSampleMs, kSampleIntervalMs)) {
    lastSampleMs = nowMs;
    sampleAndReport(nowMs);
  }
}
