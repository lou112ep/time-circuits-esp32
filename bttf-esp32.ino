// From an original idea by Davide Gatti - Survival Hacking
// https://www.youtube.com/watch?v=h1pgLkXOKSA
// Ported by lou112ep - https://github.com/lou112ep for esp32
#include <TM1637Display.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h> // Added for Captive Portal
#include "driver/ledc.h"
#include <time.h>
#include <RTClib.h> // Added for RTC

// Structure for famous dates
struct FamousDate {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  const char* description;
};

// List of famous dates (movie and history)
FamousDate famousDates[] = {
  {1955, 11, 5, 22, 4, "Clock Tower Lightning Strike"},
  {2015, 10, 21, 16, 29, "Arrival in the future"},
  {1985, 10, 26, 1, 21, "First time travel"},
  {1885, 9, 2, 8, 0, "Arrival in the Old West"},
  {1969, 7, 20, 20, 17, "Apollo 11 Moon Landing"},
  {1492, 10, 12, 12, 0, "Discovery of America"},
  {1989, 11, 9, 18, 53, "Fall of the Berlin Wall"},
  {1776, 7, 4, 12, 0, "USA Declaration of Independence"},
  {1945, 5, 8, 23, 1, "End of WWII in Europe"}
};

// WiFi Configuration in Access Point mode
const char* ap_ssid = "BTTF-TimeCircuits";
const char* ap_password = "martymcfly"; // Optional password, set to NULL for an open network

// RED Displays
#define RED_CLK 21    // Common CLK for all red displays
#define RED_TIME_DIO 22
#define RED_YEAR_DIO 25
#define RED_DAY_DIO 27

// AMBER Displays
#define AMBER_CLK 17    // Common CLK for all amber displays
#define AMBER_TIME_DIO 16
#define AMBER_YEAR_DIO 4
#define AMBER_DAY_DIO 19

// GREEN Displays (new)
#define GREEN_CLK 26    // Common CLK for all green displays
#define GREEN_TIME_DIO 18
#define GREEN_YEAR_DIO 23
#define GREEN_DAY_DIO 13

// Shared I2C bus for all alphanumeric displays
#define SHARED_SDA 33
#define SHARED_SCL 32

// I2C Addresses
#define RED_MONTH_ADDR 0x70    // No jumpers
#define AMBER_MONTH_ADDR 0x74  // A2 jumpered
#define GREEN_MONTH_ADDR 0x72  // A1 jumpered

// Speaker pin definition
#define BUZZER_PIN 5

// PWM configuration for the buzzer
#define BUZZER_CHANNEL 0
#define BUZZER_FREQ 800    // Beep frequency in Hz
#define BUZZER_RESOLUTION 8

// NTP Configuration - REMOVED
// const char* ntpServer = "pool.ntp.org";
// const long  gmtOffset_sec = 3600;     // Timezone (3600 = GMT+1)
// const int   daylightOffset_sec = 0;    // Modified: 0 because DST is already handled
// unsigned long lastNtpSync = 0;
// const unsigned long ntpSyncInterval = 3600000; // Sync every hour (in milliseconds)

// RED display objects initialization
TM1637Display displayRedTime(RED_CLK, RED_TIME_DIO);
TM1637Display displayRedYear(RED_CLK, RED_YEAR_DIO);
TM1637Display displayRedDay(RED_CLK, RED_DAY_DIO);
Adafruit_AlphaNum4 displayRedMonth = Adafruit_AlphaNum4();

// AMBER display objects initialization
TM1637Display displayAmberTime(AMBER_CLK, AMBER_TIME_DIO);
TM1637Display displayAmberYear(AMBER_CLK, AMBER_YEAR_DIO);
TM1637Display displayAmberDay(AMBER_CLK, AMBER_DAY_DIO);
Adafruit_AlphaNum4 displayAmberMonth = Adafruit_AlphaNum4();

// GREEN display objects initialization
TM1637Display displayGreenTime(GREEN_CLK, GREEN_TIME_DIO);
TM1637Display displayGreenYear(GREEN_CLK, GREEN_YEAR_DIO);
TM1637Display displayGreenDay(GREEN_CLK, GREEN_DAY_DIO);
Adafruit_AlphaNum4 displayGreenMonth = Adafruit_AlphaNum4();

// Variable to store the selected color
String selectedColor = "red";  // Default: red

// WebServer initialization on port 80
WebServer server(80);
DNSServer dnsServer; // DNS object for the Captive Portal
RTC_DS3231 rtc; // Object for the RTC module

// Global variables for the colon blink
unsigned long previousMillis = 0;
const long blinkInterval = 500;    // Blink interval
bool colonOn = false;              // Current state of the colon
String redTimeValue = "";         // Red time value
String amberTimeValue = "";       // Amber time value
String greenTimeValue = "";       // Green time value

// Variables to keep track of the last displayed minute
unsigned long lastMinuteUpdate = 0;
int lastMinuteDisplayed = -1;

// Buffer variables for setting the RTC date
int greenYear, greenMonth, greenDay, greenHour, greenMinute;

// HTML page with integrated CSS and JavaScript
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; margin: 0px auto; padding: 20px; background-color: #fffbf3;}
    .keypad { display: grid; grid-template-columns: repeat(3, 1fr); max-width: 300px; margin: 20px auto; gap: 10px; }
    .key { background-color: #ebe5d8; border: none; border-radius: 5px; padding: 20px; font-size: 24px; cursor: pointer; }
    .key:active { background-color: #ddd; }
    #display { font-size: 32px; margin: 20px; padding: 10px; background-color: #eee; border-radius: 5px; }
    .controls { margin: 20px; }
    .control-btn { padding: 10px 20px; margin: 5px; font-size: 18px; }
    #inputMode { font-size: 24px; margin: 10px; color: #666; }
    .color-selector { margin: 20px auto; max-width: 300px; display: flex; justify-content: space-around; }
    .color-btn { 
      width: 80px; 
      height: 40px; 
      border: 2px solid rgba(0,0,0,0.3); 
      border-radius: 5px; 
      cursor: pointer;
      transition: all 0.3s ease;
    }
    .color-btn.active { 
      transform: scale(1.05);
      border-color: white;
      box-shadow: 0 0 15px currentColor, 0 0 5px rgba(255,255,255,0.8) inset;
      filter: brightness(1.3);
    }
    .red-btn { background-color: #e74c3c; }
    .amber-btn { background-color: #e67e22; }
    .green-btn { background-color: #27ae60; }
  </style>
</head>
<body>

  <div class="color-selector">
    <button id="redBtn" class="color-btn red-btn active" onclick="selectColor('red')"></button>
    <button id="greenBtn" class="color-btn green-btn" onclick="selectColor('green')"></button>
    <button id="amberBtn" class="color-btn amber-btn" onclick="selectColor('amber')"></button>
  </div>
  
  <div id="inputMode">MONTH</div>
  <div id="display">0000</div>
  
  <div class="keypad">
    <button class="key" onclick="addNumber('1')">1</button>
    <button class="key" onclick="addNumber('2')">2</button>
    <button class="key" onclick="addNumber('3')">3</button>
    <button class="key" onclick="addNumber('4')">4</button>
    <button class="key" onclick="addNumber('5')">5</button>
    <button class="key" onclick="addNumber('6')">6</button>
    <button class="key" onclick="addNumber('7')">7</button>
    <button class="key" onclick="addNumber('8')">8</button>
    <button class="key" onclick="addNumber('9')">9</button>
    <button class="key" onclick="clearDisplay()">C</button>
    <button class="key" onclick="addNumber('0')">0</button>
    <button class="key" onclick="sendCurrentValue()">OK</button>
  </div>

  <script>
    let displayValue = '';
    let inputMode = 'month';
    let monthValue = '', dayValue = '', yearValue = '', timeValue = '';
    let selectedColor = 'red'; // Default: red
    let audioContext = null;

    // DTMF frequencies for each key
    const DTMF_FREQUENCIES = {
      '1': [697, 1209], '2': [697, 1336], '3': [697, 1477],
      '4': [770, 1209], '5': [770, 1336], '6': [770, 1477],
      '7': [852, 1209], '8': [852, 1336], '9': [852, 1477],
      '0': [941, 1336], '*': [941, 1209], '#': [941, 1477]
    };

    function playDTMF(key) {
      if (!audioContext) {
        audioContext = new (window.AudioContext || window.webkitAudioContext)();
      }

      if (DTMF_FREQUENCIES[key]) {
        const [freq1, freq2] = DTMF_FREQUENCIES[key];
        const duration = 0.1;  // sound duration in seconds
        
        const osc1 = audioContext.createOscillator();
        const osc2 = audioContext.createOscillator();
        const gainNode = audioContext.createGain();
        
        gainNode.gain.value = 0.1;  // reduced volume
        
        osc1.connect(gainNode);
        osc2.connect(gainNode);
        gainNode.connect(audioContext.destination);
        
        osc1.frequency.value = freq1;
        osc2.frequency.value = freq2;
        
        osc1.start();
        osc2.start();
        
        setTimeout(() => {
          osc1.stop();
          osc2.stop();
        }, duration * 1000);
      }
    }

    function selectColor(color) {
      selectedColor = color;
      document.getElementById('redBtn').classList.remove('active');
      document.getElementById('greenBtn').classList.remove('active');
      document.getElementById('amberBtn').classList.remove('active');
      
      document.getElementById(color + 'Btn').classList.add('active');
    }
    
    function updateInputMode() {
      let message = '';
      switch(inputMode) {
        case 'month': message = 'MONTH'; break;
        case 'day': message = 'DAY'; break;
        case 'year': message = 'YEAR'; break;
        case 'time': message = 'HOUR : MIN'; break;
      }
      document.getElementById('inputMode').innerText = message;
    }
    
    function addNumber(num) {
      let maxLength = (inputMode === 'year' || inputMode === 'time') ? 4 : 2;
      
      if(displayValue.length < maxLength) {
        displayValue += num;
        updateDisplay();
        playDTMF(num);
        
        if(displayValue.length === maxLength) {
          sendCurrentValue();
        }
      }
    }
    
    function sendCurrentValue() {
      const num = parseInt(displayValue || '0');
      const colorParam = '&color=' + selectedColor;
      
      switch(inputMode) {
        case 'month':
          fetch('/setMonth?value=' + num + colorParam)
            .then(response => response.text())
            .then(() => {
              monthValue = displayValue;
              inputMode = 'day';
              displayValue = '';
              updateDisplay();
              updateInputMode();
            });
          break;
          
        case 'day':
          fetch('/setDay?value=' + num + colorParam)
            .then(response => response.text())
            .then(() => {
              dayValue = displayValue;
              inputMode = 'year';
              displayValue = '';
              updateDisplay();
              updateInputMode();
            });
          break;
          
        case 'year':
          fetch('/setYear?value=' + num + colorParam)
            .then(response => response.text())
            .then(() => {
              yearValue = displayValue;
              inputMode = 'time';
              displayValue = '';
              updateDisplay();
              updateInputMode();
            });
          break;
          
        case 'time':
          fetch('/setTime?value=' + num + colorParam)
            .then(response => response.text())
            .then(() => {
              timeValue = displayValue;
              inputMode = 'month'; // Return to month for the next cycle
              displayValue = '';
              updateDisplay();
              updateInputMode();
            });
          break;
      }
      playDTMF('#');
    }
    
    function clearDisplay() {
      displayValue = '';
      updateDisplay();
      playDTMF('*');
    }
    
    function updateDisplay() {
      const maxLength = (inputMode === 'year' || inputMode === 'time') ? 4 : 2;
      document.getElementById('display').innerText = displayValue.padStart(maxLength, '0');
    }
    
    // Initialize the input mode
    updateInputMode();
  </script>
</body>
</html>
)rawliteral";

// Function to emit a short beep
void shortBeep() {
  tone(BUZZER_PIN, 2000);
  delay(10);               // Reduced from 20ms to 10ms
  noTone(BUZZER_PIN);
}

// Function for numeric display animation (for time, year, and day)
void animateNumber(TM1637Display &display, int finalNumber, int digits = 4, bool withColon = false) {
  int shuffleTime = 1000;  // Restored to 1000ms 
  int updateInterval = 5;  // Kept this fast for fluidity
  int beepInterval = 50;   // New: minimum interval between beeps
  int startTime = millis();
  int lastBeepTime = 0;    // New: to track the last beep
  
  // Array to keep track of which positions have been "fixed"
  bool digitFixed[4] = {false, false, false, false};
  int finalDigits[4];
  
  // Extract final digits
  int temp = finalNumber;
  for(int i = digits-1; i >= 0; i--) {
    finalDigits[i] = temp % 10;
    temp /= 10;
  }
  
  // Shuffle phase
  while(millis() - startTime < shuffleTime) {
    uint8_t segments[4] = {0, 0, 0, 0};
    
    for(int i = 0; i < digits; i++) {
      if(!digitFixed[i]) {
        // New: check the interval between beeps
        if(millis() - lastBeepTime >= beepInterval) {
          shortBeep();
          lastBeepTime = millis();
        }
        if(digits == 4) {
          segments[i] = display.encodeDigit(random(10));
        } else {
          segments[i+1] = display.encodeDigit(random(10));
        }
      } else {
        // Keep the final number for already fixed positions
        if(digits == 4) {
          segments[i] = display.encodeDigit(finalDigits[i]);
        } else {
          segments[i+1] = display.encodeDigit(finalDigits[i]);
        }
      }
    }
    
    // Show segments
    display.setSegments(segments);
    if(withColon && (millis() % 1000 < 500)) {
      display.showNumberDecEx(finalNumber, 0b01000000, true);
    }
    
    delay(updateInterval);
    
    // Progressively fix digits from left to right
    int elapsed = millis() - startTime;
    for(int i = 0; i < digits; i++) {
      if(elapsed > (shuffleTime/digits * i) && !digitFixed[i]) {
        digitFixed[i] = true;
      }
    }
  }
  
  // Show the final number
  if(withColon) {
    display.showNumberDecEx(finalNumber, 0b01000000, true);
  } else if(digits == 4) {
    display.showNumberDec(finalNumber, false);
  } else {
    displayDayCentered(finalNumber, display);
  }
}

// Function for alphanumeric display animation (month)
void animateMonth(const char* monthStr, Adafruit_AlphaNum4 &display) {
  int shuffleTime = 1000;  // Restored to 1000ms
  int updateInterval = 10; // Kept this fast for fluidity
  int startTime = millis();
  int len = strlen(monthStr);
  int startPos = 4 - len;  // For right alignment
  
  // Array to keep track of which positions have been "fixed"
  bool charFixed[4] = {false, false, false, false};
  
  // Shuffle phase
  while(millis() - startTime < shuffleTime) {
    // Generate random characters and beep
    for(int i = 0; i < len; i++) {
      if(!charFixed[i]) {
        shortBeep();  // Beep for each letter change
        display.writeDigitAscii(startPos + i, random(65, 90)); // Uppercase letters A-Z
      } else {
        display.writeDigitAscii(startPos + i, monthStr[i]);
      }
    }
    display.writeDisplay();
    delay(updateInterval);
    
    // Progressively fix characters from left to right
    int elapsed = millis() - startTime;
    for(int i = 0; i < len; i++) {
      if(elapsed > (shuffleTime/len * i) && !charFixed[i]) {
        charFixed[i] = true;
      }
    }
  }
  
  // Show the final text
  displayMonthRight(monthStr, display);
}

// Function to display the month right-aligned
void displayMonthRight(const char* monthStr, Adafruit_AlphaNum4 &display) {
  display.clear();
  int len = strlen(monthStr);
  int startPos = 4 - len;  // Calculate the starting position for right alignment
  
  // Fill initial positions with spaces
  for(int i = 0; i < startPos; i++) {
    display.writeDigitAscii(i, ' ');
  }
  
  // Write the month starting from the calculated position
  for(int i = 0; i < len && i < 4; i++) {
    display.writeDigitAscii(startPos + i, monthStr[i]);
  }
  
  display.writeDisplay();
}

// Function to display the day centered in the display
void displayDayCentered(int day, TM1637Display &display) {
  uint8_t segments[] = { 0, 0, 0, 0 };  // Array for the 4 digits
  
  // Convert the day into two digits
  int tens = day / 10;
  int ones = day % 10;
  
  // Position the digits in the center (positions 1 and 2)
  segments[1] = display.encodeDigit(tens);
  segments[2] = display.encodeDigit(ones);
  
  // Leave outer positions (0 and 3) blank
  segments[0] = 0;
  segments[3] = 0;
  
  // Display on the screen
  display.setSegments(segments);
}

// Function to animate all displays in sequence
void animateAllDisplays(int year = 2024, int month = 1, int day = 31, int time = 1234) {
  // Default values if not specified
  const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
                         "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
  
  // Convert month to string
  const char* monthStr = months[month-1];
  
  // RED Sequence
  Serial.println("Animating RED displays...");
  
  // Red month
  animateMonth(monthStr, displayRedMonth);
  delay(200);  // Pause between animations
  
  // Red day
  animateNumber(displayRedDay, day, 2);
  delay(200);
  
  // Red year
  animateNumber(displayRedYear, year);
  delay(200);
  
  // Red time
  animateNumber(displayRedTime, time, 4, true);
  redTimeValue = String(time);
  delay(500);  // Longer pause between colors
  
  // GREEN Sequence
  Serial.println("Animating GREEN displays...");
  
  // Green month
  animateMonth(monthStr, displayGreenMonth);
  delay(200);
  
  // Green day
  animateNumber(displayGreenDay, day, 2);
  delay(200);
  
  // Green year
  animateNumber(displayGreenYear, year);
  delay(200);
  
  // Green time
  animateNumber(displayGreenTime, time, 4, true);
  greenTimeValue = String(time);
  delay(500);  // Longer pause between colors
  
  // AMBER Sequence
  Serial.println("Animating AMBER displays...");
  
  // Amber month
  animateMonth(monthStr, displayAmberMonth);
  delay(200);
  
  // Amber day
  animateNumber(displayAmberDay, day, 2);
  delay(200);
  
  // Amber year
  animateNumber(displayAmberYear, year);
  delay(200);
  
  // Amber time
  animateNumber(displayAmberTime, time, 4, true);
  amberTimeValue = String(time);
  
  // Reset variables for blinking
  previousMillis = 0;
  colonOn = true;
  
  Serial.println("Sequence complete!");
}

// Function to update the blinking colon on all displays
void updateTimeWithBlink() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;
    colonOn = !colonOn;  // Invert the colon state
    
    // Update all time displays with the colon
    if(redTimeValue.length() > 0) {
      displayRedTime.showNumberDecEx(redTimeValue.toInt(), colonOn ? 0b01000000 : 0, true);
    }
    
    if(amberTimeValue.length() > 0) {
      displayAmberTime.showNumberDecEx(amberTimeValue.toInt(), colonOn ? 0b01000000 : 0, true);
    }
    
    if(greenTimeValue.length() > 0) {
      displayGreenTime.showNumberDecEx(greenTimeValue.toInt(), colonOn ? 0b01000000 : 0, true);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize the random number generator using the noise on an analog pin
  // It's a simple way to get a different random seed on each boot
  randomSeed(analogRead(A0));
  
  // Initialize I2C for the alphanumeric displays
  Wire.begin(SHARED_SDA, SHARED_SCL);

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC module!");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time to compile time...");
    // Set the RTC to the date and time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  // Initialize RED displays
  displayRedTime.setBrightness(5);
  displayRedTime.clear();
  
  displayRedYear.setBrightness(5);
  displayRedYear.clear();
  
  displayRedDay.setBrightness(5);
  displayRedDay.clear();
  
  displayRedMonth.begin(RED_MONTH_ADDR);
  displayRedMonth.setBrightness(15);
  displayRedMonth.clear();
  
  // Initialize AMBER displays
  displayAmberTime.setBrightness(5);
  displayAmberTime.clear();
  
  displayAmberYear.setBrightness(5);
  displayAmberYear.clear();
  
  displayAmberDay.setBrightness(5);
  displayAmberDay.clear();
  
  displayAmberMonth.begin(AMBER_MONTH_ADDR);
  displayAmberMonth.setBrightness(15);
  displayAmberMonth.clear();
  
  // Initialize GREEN displays
  displayGreenTime.setBrightness(5);
  displayGreenTime.clear();
  
  displayGreenYear.setBrightness(5);
  displayGreenYear.clear();
  
  displayGreenDay.setBrightness(5);
  displayGreenDay.clear();
  
  displayGreenMonth.begin(GREEN_MONTH_ADDR);
  displayGreenMonth.setBrightness(15);
  displayGreenMonth.clear();
  
  // Configure buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  
  // WiFi connection -> Create Access Point
  Serial.println("Configuring Access Point...");
  WiFi.softAP(ap_ssid, ap_password);
  
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("Access Point started. IP address: ");
  Serial.println(myIP);

  // Start DNS server for Captive Portal
  dnsServer.start(53, "*", myIP);
  Serial.println("DNS server for Captive Portal started");
  
  // Start the server and configure the various endpoints
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", index_html);
  });

  // Add a "catch-all" handler for the Captive Portal
  // If the client requests a page that doesn't exist, we redirect to the main page.
  server.onNotFound([]() {
    server.send(200, "text/html", index_html);
  });
  
  server.on("/setMonth", HTTP_GET, []() {
    String monthStr = server.arg("value");
    String color = server.arg("color");
    int month = monthStr.toInt();
    const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
                           "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
    if(month >= 1 && month <= 12) {
      if(color == "red") {
        animateMonth(months[month-1], displayRedMonth);
      } else if(color == "green") {
        greenMonth = month; // Buffer the month for the RTC
        animateMonth(months[month-1], displayGreenMonth);
      } else {
        animateMonth(months[month-1], displayAmberMonth);
      }
    }
    server.send(200, "text/plain", "OK");
  });
  
  server.on("/setDay", HTTP_GET, []() {
    String dayStr = server.arg("value");
    String color = server.arg("color");
    int day = dayStr.toInt();
    if(day >= 1 && day <= 31) {
      if(color == "red") {
        animateNumber(displayRedDay, day, 2);
      } else if(color == "green") {
        greenDay = day; // Buffer the day for the RTC
        animateNumber(displayGreenDay, day, 2);
      } else {
        animateNumber(displayAmberDay, day, 2);
      }
    }
    server.send(200, "text/plain", "OK");
  });
  
  server.on("/setYear", HTTP_GET, []() {
    String yearStr = server.arg("value");
    String color = server.arg("color");
    int year = yearStr.toInt();
    if(color == "red") {
      animateNumber(displayRedYear, year);
    } else if(color == "green") {
      greenYear = year; // Buffer the year for the RTC
      animateNumber(displayGreenYear, year);
    } else {
      animateNumber(displayAmberYear, year);
    }
    server.send(200, "text/plain", "OK");
  });
  
  server.on("/setTime", HTTP_GET, []() {
    String timeStr = server.arg("value");
    String color = server.arg("color");
    int time = timeStr.toInt();
    
    if(color == "red") {
      redTimeValue = timeStr;  // Store the value for blinking
      animateNumber(displayRedTime, time, 4, true);
    } else if(color == "green") {
      greenTimeValue = timeStr;  // Store the value for blinking
      
      // Set the time on the RTC
      greenHour = time / 100;
      greenMinute = time % 100;
      rtc.adjust(DateTime(greenYear, greenMonth, greenDay, greenHour, greenMinute, 0));
      Serial.println("RTC updated with the new time!");
      
      animateNumber(displayGreenTime, time, 4, true);
    } else { // amber
      amberTimeValue = timeStr;  // Store the value for blinking
      animateNumber(displayAmberTime, time, 4, true);
    }
    server.send(200, "text/plain", "OK");
  });
  
  server.on("/animateAll", HTTP_GET, []() {
    int year = server.arg("year").toInt() > 0 ? server.arg("year").toInt() : 2024;
    int month = server.arg("month").toInt() > 0 ? server.arg("month").toInt() : 1;
    int day = server.arg("day").toInt() > 0 ? server.arg("day").toInt() : 1;
    int time = server.arg("time").toInt() >= 0 ? server.arg("time").toInt() : 0;
    
    // Start the animation
    animateAllDisplays(year, month, day, time);
    
    server.send(200, "text/plain", "Animation complete!");
  });
  
  server.begin();
  
  // --- NEW STARTUP LOGIC WITH RANDOM DATES ---
  
  // Month names for display
  const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
                         "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

  // Calculate how many famous dates we have
  int numDates = sizeof(famousDates) / sizeof(famousDates[0]);

  // Choose two different random indexes for red and amber displays
  int redIndex = random(numDates);
  int amberIndex = random(numDates);
  while (amberIndex == redIndex) {
    amberIndex = random(numDates);
  }

  // Extract dates from the array
  FamousDate redDate = famousDates[redIndex];
  FamousDate amberDate = famousDates[amberIndex];

  Serial.println("Random RED date: " + String(redDate.description));
  Serial.println("Random AMBER date: " + String(amberDate.description));

  // Animate RED displays (Destination Time)
  animateMonth(months[redDate.month - 1], displayRedMonth);
  delay(200);
  animateNumber(displayRedDay, redDate.day, 2);
  delay(200);
  animateNumber(displayRedYear, redDate.year);
  delay(200);
  int redTime = redDate.hour * 100 + redDate.minute;
  animateNumber(displayRedTime, redTime, 4, true);
  redTimeValue = String(redTime);
  delay(500);

  // Animate AMBER displays (Last Time Departed)
  animateMonth(months[amberDate.month - 1], displayAmberMonth);
  delay(200);
  animateNumber(displayAmberDay, amberDate.day, 2);
  delay(200);
  animateNumber(displayAmberYear, amberDate.year);
  delay(200);
  int amberTime = amberDate.hour * 100 + amberDate.minute;
  animateNumber(displayAmberTime, amberTime, 4, true);
  amberTimeValue = String(amberTime);

  // The old static animation is removed
  // animateAllDisplays(2024, 1, 1, 0000);
  
  // Clear all variables (except those just set)
  greenTimeValue = "";
  
  // Update the GREEN display (Present Time) with the current time from the RTC
  DateTime now = rtc.now();
  int year = now.year();
  int month = now.month();
  int day = now.day();
  int timeValue = now.hour() * 100 + now.minute();

  displayMonthRight(months[month-1], displayGreenMonth);
  displayDayCentered(day, displayGreenDay);
  displayGreenYear.showNumberDec(year, false);
  displayGreenTime.showNumberDecEx(timeValue, 0b01000000, true);
  greenTimeValue = String(timeValue);
  lastMinuteDisplayed = now.minute();
}

void loop() {
  dnsServer.processNextRequest(); // Handle DNS requests for the Captive Portal
  server.handleClient();
  updateTimeWithBlink();  // Update the blinking colon for all displays
  
  // All NTP-based logic (getLocalTime) is removed
  // Add logic to update the green display with the RTC
  DateTime now = rtc.now();

  // Per-minute update
  if (now.minute() != lastMinuteDisplayed) {
    // Format the time for the display (HHMM)
    int hours = now.hour();
    int minutes = now.minute();
    int timeValue = hours * 100 + minutes;
    
    // Update only the green time display, without animation
    displayGreenTime.showNumberDecEx(timeValue, 0b01000000, true);
    greenTimeValue = String(timeValue);
    
    // If day, month, or year change, update those as well
    static int lastDay = -1, lastMonth = -1, lastYear = -1;
    
    if (now.day() != lastDay) {
      displayDayCentered(now.day(), displayGreenDay);
      lastDay = now.day();
    }
    
    if (now.month() != lastMonth) {
      const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
                             "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
      displayMonthRight(months[now.month()-1], displayGreenMonth);
      lastMonth = now.month();
    }
    
    if (now.year() != lastYear) {
      displayGreenYear.showNumberDec(now.year());
      lastYear = now.year();
    }
    
    lastMinuteDisplayed = now.minute();
  }
}
