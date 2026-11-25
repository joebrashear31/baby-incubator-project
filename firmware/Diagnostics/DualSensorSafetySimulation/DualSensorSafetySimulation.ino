namespace {

constexpr uint8_t kHeaterGatePin = 9;
constexpr uint32_t kStageDurationMs = 4000;
constexpr float kHeaterOnBelowC = 25.8F;
constexpr float kHeaterOffAboveC = 26.2F;
constexpr float kHighTemperatureWarningC = 27.0F;
constexpr float kHighTemperatureShutdownC = 28.0F;
constexpr float kShutdownRestartBelowC = 27.0F;

struct SimulationStage {
  float primaryC;
  float crossCheckC;
  const char *name;
};

const SimulationStage kStages[] = {
    {25.0F, 25.2F, "BELOW_TARGET"},
    {26.3F, 26.4F, "ABOVE_TARGET"},
    {27.2F, 27.3F, "HIGH_WARNING"},
    {28.1F, 28.2F, "TRIGGER_SHUTDOWN"},
    {27.5F, 27.4F, "LATCHED_COOLDOWN"},
    {26.8F, 26.9F, "LATCH_CLEARED"},
    {25.5F, 25.6F, "HEATING_RESTART"},
};

constexpr size_t kStageCount = sizeof(kStages) / sizeof(kStages[0]);
bool heaterEnabled = false;
bool shutdownLatched = false;
size_t lastReportedStage = kStageCount;

void setHeaterEnabled(bool enabled) {
  heaterEnabled = enabled;
  digitalWrite(kHeaterGatePin, heaterEnabled ? HIGH : LOW);
}

void updateControl(float primaryC, float crossCheckC) {
  const float highestC = max(primaryC, crossCheckC);

  if (highestC >= kHighTemperatureShutdownC) {
    shutdownLatched = true;
  } else if (shutdownLatched && primaryC < kShutdownRestartBelowC &&
             crossCheckC < kShutdownRestartBelowC) {
    shutdownLatched = false;
  }

  if (shutdownLatched) {
    setHeaterEnabled(false);
  } else if (primaryC <= kHeaterOnBelowC) {
    setHeaterEnabled(true);
  } else if (primaryC >= kHeaterOffAboveC) {
    setHeaterEnabled(false);
  }
}

const __FlashStringHelper *stateFor(float primaryC, float crossCheckC) {
  if (shutdownLatched) {
    return F("SHUTDOWN_LATCHED");
  }
  if (max(primaryC, crossCheckC) >= kHighTemperatureWarningC) {
    return F("HIGH_WARNING");
  }
  return F("NORMAL");
}

void reportStage(size_t stageIndex) {
  const SimulationStage &stage = kStages[stageIndex];
  updateControl(stage.primaryC, stage.crossCheckC);

  Serial.print(stageIndex + 1);
  Serial.print(',');
  Serial.print(stage.name);
  Serial.print(',');
  Serial.print(stage.primaryC, 1);
  Serial.print(',');
  Serial.print(stage.crossCheckC, 1);
  Serial.print(',');
  Serial.print(heaterEnabled ? F("ON,") : F("OFF,"));
  Serial.println(stateFor(stage.primaryC, stage.crossCheckC));
}

}  // namespace

void setup() {
  // Keep the output off before the simulation begins.
  digitalWrite(kHeaterGatePin, LOW);
  pinMode(kHeaterGatePin, OUTPUT);

  Serial.begin(115200);
  Serial.println(F("stage,name,ds18b20_c,tmp117_c,heater_state,safety_state"));
}

void loop() {
  const size_t stageIndex = (millis() / kStageDurationMs) % kStageCount;
  if (stageIndex != lastReportedStage) {
    lastReportedStage = stageIndex;
    reportStage(stageIndex);
  }
}
