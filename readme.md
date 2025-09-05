# Back to the Future Time Circuits Display (Standalone ESP32 Version)

![BTTF Time Circuits](/images/time-circuits.jpg)

A fully functional, standalone recreation of the iconic time circuits display from the Back to the Future movie trilogy, powered by an ESP32. This version operates completely offline, creating its own WiFi network for configuration. Show destination, present, and last departure time just like Doc Brown's DeLorean!

## Features

- **Three Complete Display Sets**:
  - **Red**: Destination time (where you're going)
  - **Green**: Present time (driven by a persistent Real-Time Clock)
  - **Amber**: Last departure time (where you've been)

- **Standalone & Offline Operation**:
  - **Access Point Mode**: The device creates its own WiFi network (`BTTF-TimeCircuits`). No external internet connection or router is required.
  - **Captive Portal**: Connect to the WiFi, and the configuration page will open automatically in your browser or via a system notification. No IP address to remember!
  - **RTC Timekeeping**: An integrated DS3231 Real-Time Clock keeps the "Present Time" accurate and persistent, even when the device is powered off.

- **Web-based Control Interface**:
  - Mobile-friendly design with a touch-tone style keypad and authentic DTMF sounds.
  - Color selection buttons for controlling each display set.

![Interface](/images/interface.jpg)

- **Visual & Sound Effects**:
  - **Random Startup Sequence**: On boot, the Destination and Last Departure displays show random famous dates from history and the movie.
  - Slot-machine style animations when changing values.
  - Blinking colons on all time displays.
  - Authentic sound effects for keypad presses and animations.

## Hardware Requirements

- ESP32 WROOM Development Board
- **DS3231 RTC Module**
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

### Common I2C Bus (for alphanumeric displays and RTC)
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
- RTC Module: 0x68 (usually fixed)

## Software Requirements

- Arduino IDE
- Required Libraries:
  - `TM1637Display`
  - `Adafruit_GFX`
  - `Adafruit_LEDBackpack`
  - `RTClib` by Adafruit
  - ESP32 Core libraries (WiFi, WebServer, DNSServer)

## Setup and Usage

1. **Hardware Assembly**:
   - Connect all displays and the RTC module according to the pin configuration.
   - Connect the piezo buzzer to pin 5 and GND.
   - Power the ESP32 via USB or an external 5V supply.

2. **Software Installation**:
   - Install Arduino IDE and the ESP32 board manager.
   - Install all the required libraries via the Arduino Library Manager.
   - Open the project `.ino` file.

3. **Upload the Code**:
   - Connect the ESP32 to your computer.
   - Select the correct board and port in the Arduino IDE.
   - Upload the sketch.

4. **Using the Time Circuits**:
   - On your phone or computer, search for WiFi networks and connect to **`BTTF-TimeCircuits`**.
   - The password is **`martymcfly`**.
   - After connecting, a system notification should appear to "Sign in to network". Tap it, and the control panel will open automatically.
   - If no notification appears, open a web browser and try to navigate to any website (e.g., `a.com`), and you will be redirected.
   - You can now set the Destination (Red) and Last Departure (Amber) times. The Present (Green) time will update automatically from the RTC.

## Troubleshooting

- **Displays Not Lighting Up**: Check wiring connections and power supply. Ensure the 5V rail can supply enough current.
- **WiFi Network Not Visible**: Check that the sketch uploaded correctly and the ESP32 is powered on.
- **Control Panel Not Opening Automatically**:
  - Make sure you are connected to the `BTTF-TimeCircuits` WiFi network.
  - **Disable Mobile/Cellular Data** on your phone, as it can interfere with the captive portal redirection.
  - Try opening a browser and navigating to a non-HTTPS site like `http://example.com`.
- **Green Time is Incorrect**: The RTC may have lost power. Setting the green time via the web interface will permanently save it to the RTC.

## License

This project is open-source under the MIT License.

## Acknowledgments

- Universal Pictures for the Back to the Future trilogy
- Davide Gatti for the original inspiration
- The Arduino and ESP32 communities
- Library authors for TM1637, HT16K33, and RTC displays

---

*"Your future is whatever you make it, so make it a good one!"* – Doc Brown
