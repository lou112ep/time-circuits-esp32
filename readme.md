# Back to the Future Time Circuits Display

![BTTF Time Circuits](/images/time-circuits.jpg)

A fully functional recreation of the iconic time circuits display from the Back to the Future movie trilogy, powered by an ESP32 microcontroller. Show destination, present, and last departure time just like Doc Brown's DeLorean!

## Features

- **Three Complete Display Sets**:
  - **Red**: Destination time (where you're going)
  - **Green**: Present time (automatically synchronized with real time)
  - **Amber**: Last departure time (where you've been)

- **Web-based Control Interface**:
  - Mobile-friendly design
  - Touch-tone style keypad with authentic DTMF sounds
  - Color selection buttons for controlling each display set

![Interface](/images/interface.jpg)

- **Visual Effects**:
  - Slot-machine style animations when changing values
  - Blinking colons on all time displays
  - Full sequence animation at startup
  - Authentic sound effects

- **Smart Features**:
  - NTP time synchronization over WiFi
  - Automatic hourly time corrections
  - Brightness adjustment based on time of day (100% during day, 65% at night)

## Hardware Requirements

- ESP32 WROOM Development Board
- 9× TM1637 4-digit 7-segment displays:
  - 3× for times (red, green, amber)
  - 3× for years (red, green, amber)
  - 3× for days (red, green, amber)
- 3× HT16K33 alphanumeric displays (for months)
- Piezo buzzer (connected to pin 5)
- Jumper wires
- 5V power supply (2A recommended)
- Breadboard or custom PCB

## Pin Configuration

### Common I2C Bus (for alphanumeric displays)
- SDA: Pin 33
- SCL: Pin 32

### Display Pin Assignments
```
// RED DISPLAYS
RED_CLK: 21       // Common clock for all red displays
RED_TIME_DIO: 22  // Time display
RED_YEAR_DIO: 25  // Year display
RED_DAY_DIO: 27   // Day display

// GREEN DISPLAYS
GREEN_CLK: 26     // Common clock for all green displays
GREEN_TIME_DIO: 18 // Time display
GREEN_YEAR_DIO: 23 // Year display
GREEN_DAY_DIO: 13  // Day display

// AMBER DISPLAYS
AMBER_CLK: 17     // Common clock for all amber displays
AMBER_TIME_DIO: 16 // Time display
AMBER_YEAR_DIO: 4  // Year display
AMBER_DAY_DIO: 19  // Day display

// BUZZER
BUZZER_PIN: 5
```

### I2C Addresses
- Red Month Display: 0x70 (no jumpers)
- Green Month Display: 0x72 (A1 jumpered)
- Amber Month Display: 0x74 (A2 jumpered)

## Software Requirements

- Arduino IDE
- Required Libraries:
  - TM1637Display
  - Adafruit_GFX
  - Adafruit_LEDBackpack
  - ESP32 libraries (WiFi, WebServer)

## Setup Instructions

1. **Hardware Assembly**:
   - Connect all displays according to the pin configuration
   - Connect the piezo buzzer to pin 5 and GND
   - Power the ESP32 via USB or external 5V supply

2. **Software Installation**:
   - Install Arduino IDE
   - Install the required libraries via Library Manager
   - Open the project file (.ino)
   
3. **Configuration**:
   - Edit WiFi credentials in the code:
     ```cpp
     const char* ssid = "your-ssid";
     const char* password = "your-password";
     ```
   - Set your timezone (if different from Central European Time):
     ```cpp
     const long gmtOffset_sec = 3600;     // GMT+1
     const int daylightOffset_sec = 0;    // Already handled by system
     ```

4. **Upload the Code**:
   - Connect ESP32 to your computer
   - Select the correct board and port in Arduino IDE
   - Upload the sketch
   - Open Serial Monitor at 115200 baud to see the IP address

## Usage

1. **Accessing the Web Interface**:
   - Connect to the same WiFi network as the ESP32
   - Open a browser and navigate to the IP address shown in Serial Monitor
   - You should see the control panel with the keypad and color buttons

2. **Setting Date and Time**:
   - Select a color by clicking the red, green, or amber button
   - Enter values in sequence when prompted:
     1. Month (01-12)
     2. Day (01-31)
     3. Year (4 digits)
     4. Time (HHMM in 24h format)
   - Values will be displayed with an animation on the corresponding displays

3. **Automatic Time Updates**:
   - The green displays automatically show the current time
   - Time is synchronized every hour at the start of the hour
   - Brightness automatically adjusts based on time of day

## Troubleshooting

- **Displays Not Lighting Up**: Check wiring connections and power supply
- **WiFi Not Connecting**: Verify SSID and password
- **Web Interface Not Loading**: Check IP address and network connection
- **Time Not Updating**: Ensure NTP server is reachable
- **Missing Month Display**: Verify I2C addresses and wiring

## Future Enhancements

- Add RTC module for timekeeping without WiFi
- Create a 3D-printable enclosure
- Add sound effects from the movie
- Implement destination time input validation
- Add flux capacitor simulation

## License

This project is open-source under the MIT License. See LICENSE file for details.

## Acknowledgments

- Universal Pictures for the Back to the Future trilogy
- The Arduino and ESP32 communities
- Library authors for TM1637 and HT16K33 displays

---

*"Your future is whatever you make it, so make it a good one!"* – Doc Brown
