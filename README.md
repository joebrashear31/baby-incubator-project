# Arduino Incubator Prototype

This repository contains the embedded software for a classroom incubator proof
of concept. The current milestone reads one Adafruit TMP117 temperature sensor
with an Arduino Uno R3 and reports the result over USB serial.

## Current hardware

- Arduino Uno R3
- Adafruit TMP117 temperature sensor breakout
- USB cable
- Four jumper wires, or one compatible STEMMA QT cable

Leave the 12 V heater supply disconnected during this milestone.

## Wiring

Disconnect USB power before changing the wiring.

| TMP117 | Arduino Uno R3 |
| --- | --- |
| VIN | 5V |
| GND | GND |
| SDA | A4/SDA |
| SCL | A5/SCL |

If you are using individual jumper wires, solder the included pin header to the
TMP117 breakout before placing it on a solderless breadboard. A STEMMA QT cable
with appropriate Uno-side connectors can be used instead.

## Arduino libraries

Install these libraries through the Arduino IDE Library Manager:

1. `Adafruit TMP117`
2. `Adafruit BusIO`
3. `Adafruit Unified Sensor`

The sketch is located at
[`firmware/Incubator/Incubator.ino`](firmware/Incubator/Incubator.ino).

## Run the bench test

1. Connect the TMP117 using the wiring table above.
2. Connect the Uno to the computer over USB.
3. Open `firmware/Incubator/Incubator.ino` in the Arduino IDE.
4. Select **Arduino Uno** and the Uno's serial port.
5. Compile and upload the sketch.
6. Open Serial Monitor at **115200 baud**.

The sketch prints one CSV row per second:

```text
elapsed_ms,tmp117_c,status
0,23.742,OK
1000,23.750,OK
```

The first column is elapsed time since the Uno started. The second is the TMP117
temperature in degrees Celsius. The final column reports the sensor status.

Possible status values are:

| Status | Meaning |
| --- | --- |
| `OK` | The sensor responded and the reading was plausible. |
| `SENSOR_NOT_FOUND` | No TMP117 was available during initialization. |
| `SENSOR_INIT_FAILED` | A device responded at the address but was not initialized as a TMP117. |
| `SENSOR_DISCONNECTED` | A previously initialized sensor stopped responding. |
| `INVALID_READING` | The returned temperature was non-finite or outside the TMP117 measurement range. |

The sketch retries the sensor connection every two seconds after a missing or
disconnected sensor. It does not command a heater or implement PID control in
this milestone.

## Manual verification

Record the result of each check before adding another sensor or heater control.

| Check | Expected result | Status |
| --- | --- | --- |
| Start with the TMP117 connected | A plausible room temperature appears once per second with `OK`. | Passed — user verified on hardware |
| Bring a warm hand near the sensor | The reported temperature rises gradually. | Passed — rose from about 26 °C to 29 °C and returned to 26 °C |
| Start with the TMP117 disconnected | Rows contain a blank temperature and `SENSOR_NOT_FOUND`. | Not performed |
| Reconnect the TMP117 | The sketch retries and returns to `OK` without a reset. | Not performed |

## Planned milestones

1. Add the DS18B20 primary sensor and compare both temperature readings.
2. Implement startup checks, stale-data detection, disagreement handling, and a
   latched fault state using the built-in LED as a simulated heater.
3. Add bounded PID control and CSV fields for requested and applied heater duty.
4. Validate one fused MOSFET/heater branch on the bench.
5. Scale to the final protected heater assembly and tune the empty enclosure.

This project is a classroom demonstration and is intended for testing an empty
enclosure.
