## Main Controllers:

- **Arduino MEGA 2560** - Main microcontroller
- **ESP32** - WiFi connectivity and camera/microphone handling

## Sensors:

- **HC-SR04 Ultrasonic Sensor** - Distance measurement for obstacle detection
- **NEOM GPS Module** - Location tracking
- **Microphone** - Audio capture (connected to ESP32 analog pin)

## Actuators:

- **Vibration Motor** - Tactile feedback for alerts
- **Buzzer/Piezo Speaker** - Audio alerts

## Input Devices:

- **Power Button** - System on/off control
- **Panic Button** - Emergency SOS trigger

## Storage & Communication:

- **SD Card Module** - Data storage
- **Camera Module** - Image capture (ESP32-CAM compatible)

## Power Management:

- **Power supply/Battery pack** - System power (not explicitly coded but required)

## Pin Assignments Used:

### Arduino MEGA Pins:

- Pin 2, 3: ESP32 communication (RX, TX)
- Pin 4: Ultrasonic sensor trigger
- Pin 5: Ultrasonic sensor echo
- Pin 6: Vibration motor control
- Pin 7: Buzzer control
- Pin 8: Power button input
- Pin 9: Panic button input
- Pin 10: SD card chip select

### ESP32 Pins:

- Pin 16, 17: GPS module communication (RX, TX)
- Pin 32: Camera power control
- Pin 33: Microphone analog input
- Camera pins: (Standard ESP32-CAM pinout)

## Additional Hardware Considerations:

- **Resistors** - Pull-up resistors for buttons
- **Capacitors** - Power filtering
- **Connecting wires/PCB** - System interconnection
- **Enclosure** - Weather-resistant housing for cane mounting
- **Mounting hardware** - Brackets to attach to cane
