# Arduino Incubator Prototype

This repository contains the embedded software for a classroom incubator proof
of concept. The current milestone uses a DS18B20 as the primary temperature
sensor and an Adafruit TMP117 as an independent cross-check. The sensors control
an LED-only MOSFET bench circuit and report the result over USB serial.

## Current hardware

- Arduino Uno R3
- Adafruit TMP117 temperature sensor breakout
- Waterproof DS18B20 temperature probe
- 4.7 kohm DS18B20 data pull-up resistor
- IRLB8721 logic-level N-channel MOSFET
- 220 ohm MOSFET gate resistor
- 10 kohm gate pull-down resistor
- LED and 220 ohm to 1 kohm LED current-limiting resistor
- USB cable
- Breadboard and jumper wires

The LED represents the heater during this milestone. Leave the 12 V supply and
real heater disconnected.

## Wiring

Disconnect USB power before changing the wiring.

| TMP117 | Arduino Uno R3 |
| --- | --- |
| VIN | 5V |
| GND | GND |
| SDA | A4/SDA |
| SCL | A5/SCL |

| DS18B20 | Arduino Uno R3 |
| --- | --- |
| VDD/red | 5V |
| GND/black | GND |
| DATA/yellow or white | D2 |

Install a 4.7 kohm pull-up resistor between DATA/D2 and 5V. Probe wire colors
can vary, so confirm the seller's pinout before applying power.

The low-current MOSFET test circuit uses the following connections:

| Connection | Destination |
| --- | --- |
| Arduino D9 | 220 ohm resistor, then MOSFET gate |
| MOSFET gate | 10 kohm resistor to ground |
| MOSFET source | Arduino ground |
| MOSFET drain | LED cathode |
| LED anode | Current-limiting resistor, then Arduino 5V |

If you are using individual jumper wires, solder the included pin header to the
TMP117 breakout before placing it on a solderless breadboard. A STEMMA QT cable
with appropriate Uno-side connectors can be used instead.

## Arduino libraries

Install these libraries through the Arduino IDE Library Manager:

1. `Adafruit TMP117`
2. `Adafruit BusIO`
3. `Adafruit Unified Sensor`
4. `DallasTemperature`
5. `OneWire`

The sketch is located at
[`firmware/Incubator/Incubator.ino`](firmware/Incubator/Incubator.ino).

## Run the bench test

1. Connect both sensors using the wiring tables above.
2. Connect the Uno to the computer over USB.
3. Open `firmware/Incubator/Incubator.ino` in the Arduino IDE.
4. Select **Arduino Uno** and the Uno's serial port.
5. Compile and upload the sketch.
6. Open Serial Monitor at **115200 baud**.

The sketch prints one CSV row per second:

```text
elapsed_ms,ds18b20_c,tmp117_c,difference_c,primary_status,crosscheck_status,heater_state,temperature_state
0,23.750,23.742,0.008,OK,OK,ON,NORMAL
1000,23.750,23.750,0.000,OK,OK,ON,NORMAL
```

The first column is elapsed time since the Uno started. The next two columns
are the DS18B20 and TMP117 temperatures in degrees Celsius, followed by their
absolute difference. The remaining columns report each sensor's health, the
simulated heater output, and the temperature safety state.

Possible status values are:

| Status | Meaning |
| --- | --- |
| `OK` | The sensor responded and the reading was plausible. |
| `DS18B20_NOT_FOUND` | The primary probe is missing or disconnected. |
| `DS18B20_INVALID` | The primary reading is outside its plausible range. |
| `TMP117_NOT_FOUND` | No TMP117 was available during initialization. |
| `TMP117_INIT_FAILED` | A device responded but could not initialize as a TMP117. |
| `TMP117_DISCONNECTED` | A previously initialized TMP117 stopped responding. |
| `TMP117_INVALID` | The TMP117 reading is outside its plausible range. |

The current temporary bench profile turns the LED on at or below 25.8 C and off
at or above 26.2 C. It reports a warning at 27.0 C and latches shutdown at
28.0 C until both sensors fall below 27.0 C. Set
`kBenchTestProfileEnabled` to `false` to restore the empty-enclosure thresholds:
LED on at 34.8 C, off at 35.2 C, warning at 36.0 C, shutdown at 37.0 C, and
restart below 36.0 C. Any missing,
disconnected, or invalid sensor reading immediately turns the output off. A
difference greater than 1.0 C between the sensors also turns the output off and
reports `SENSOR_DISAGREEMENT`. The sketch retries both sensors on later samples.
It does not implement PID control or authorize connecting the real heater in
this milestone.

## Manual verification

Record the result of each check before adding another sensor or heater control.

| Check | Expected result | Status |
| --- | --- | --- |
| Start with the TMP117 connected | A plausible room temperature appears once per second with `OK`. | Passed — user verified on hardware |
| Bring a warm hand near the sensor | The reported temperature rises gradually. | Passed — rose from about 26 °C to 29 °C and returned to 26 °C |
| Run at normal room temperature | The test LED is on and CSV reports `ON,NORMAL`. | Not performed |
| Start with the TMP117 disconnected | The LED remains off and CSV reports `TMP117_NOT_FOUND,OFF,SENSOR_FAULT`. | Not performed |
| Reconnect the TMP117 | The sketch retries, returns to `OK`, and safely resumes control. | Not performed |
| Run the separate simulated-temperature sequence | Warning, shutdown latch, cooldown, and restart states follow the expected sequence. | Passed — user verified on hardware |
| Read the DS18B20 diagnostic sketch | Temperature rises when held and cools after release. | Passed — 23.75 C to about 28.06 C |
| Disconnect the DS18B20 data wire | CSV reports `PROBE_NOT_FOUND`. | Passed — user verified on hardware |
| Run the dual-sensor safety simulation | LED turns off at 26.2 C, reports a warning at 27 C, latches off at 28 C, and restarts only after the staged cooldown. | Passed — user verified the complete seven-stage sequence |

## Planned milestones

1. Verify the real TMP117 safely controls the LED-only MOSFET circuit, including
   sensor disconnection and reconnection.
2. Validate the DS18B20 primary sensor and TMP117 cross-check together using
   the LED-only MOSFET circuit.
3. Implement stale-data detection and a latched sensor
   fault state.
4. Add bounded PID control and CSV fields for requested and applied heater duty.
5. Validate one fused MOSFET/heater branch on the bench.
6. Scale to the final protected heater assembly and tune the empty enclosure.

This project is a classroom demonstration and is intended for testing an empty
enclosure.
