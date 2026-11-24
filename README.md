# Arduino Incubator Prototype

This repository contains the embedded software for a classroom incubator proof
of concept. The current milestone uses an Adafruit TMP117 temperature sensor to
control an LED-only MOSFET bench circuit and reports the result over USB serial.

## Current hardware

- Arduino Uno R3
- Adafruit TMP117 temperature sensor breakout
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
elapsed_ms,tmp117_c,sensor_status,heater_state,temperature_state
0,23.742,OK,ON,NORMAL
1000,23.750,OK,ON,NORMAL
```

The first column is elapsed time since the Uno started. The second is the TMP117
temperature in degrees Celsius. The remaining columns report sensor health,
the simulated heater output, and the temperature safety state.

Possible status values are:

| Status | Meaning |
| --- | --- |
| `OK` | The sensor responded and the reading was plausible. |
| `SENSOR_NOT_FOUND` | No TMP117 was available during initialization. |
| `SENSOR_INIT_FAILED` | A device responded at the address but was not initialized as a TMP117. |
| `SENSOR_DISCONNECTED` | A previously initialized sensor stopped responding. |
| `INVALID_READING` | The returned temperature was non-finite or outside the TMP117 measurement range. |

The LED turns on below 34.8 C and off above 35.2 C. A high-temperature warning
is reported at 36.0 C. At 37.0 C the output turns off and the shutdown remains
latched until the measured temperature falls below 36.0 C. Any missing,
disconnected, or invalid sensor reading immediately turns the output off. The
sketch retries the sensor connection every two seconds after a missing or
disconnected sensor. It does not implement PID control or authorize connecting
the real heater in this milestone.

## Manual verification

Record the result of each check before adding another sensor or heater control.

| Check | Expected result | Status |
| --- | --- | --- |
| Start with the TMP117 connected | A plausible room temperature appears once per second with `OK`. | Passed — user verified on hardware |
| Bring a warm hand near the sensor | The reported temperature rises gradually. | Passed — rose from about 26 °C to 29 °C and returned to 26 °C |
| Run at normal room temperature | The test LED is on and CSV reports `ON,NORMAL`. | Not performed |
| Start with the TMP117 disconnected | The LED remains off and CSV reports `SENSOR_NOT_FOUND,OFF,SENSOR_FAULT`. | Not performed |
| Reconnect the TMP117 | The sketch retries, returns to `OK`, and safely resumes control. | Not performed |
| Run the separate simulated-temperature sequence | Warning, shutdown latch, cooldown, and restart states follow the expected sequence. | Passed — user verified on hardware |

## Planned milestones

1. Verify the real TMP117 safely controls the LED-only MOSFET circuit, including
   sensor disconnection and reconnection.
2. Add the DS18B20 primary sensor and compare both temperature readings.
3. Implement stale-data detection, disagreement handling, and a latched sensor
   fault state.
4. Add bounded PID control and CSV fields for requested and applied heater duty.
5. Validate one fused MOSFET/heater branch on the bench.
6. Scale to the final protected heater assembly and tune the empty enclosure.

This project is a classroom demonstration and is intended for testing an empty
enclosure.
