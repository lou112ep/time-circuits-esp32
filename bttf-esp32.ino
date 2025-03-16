// From an original idea by Davide Gatti - Survival Hacking
// https://www.youtube.com/watch?v=h1pgLkXOKSA
// Ported by lou112ep - https://github.com/lou112ep for esp32
#include <TM1637Display.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include "driver/ledc.h"
#include <time.h>

// Configurazione WiFi
const char* ssid = "your-ssid";      // Inserisci il nome della tua rete WiFi
const char* password = "your-password";  // Inserisci la password della tua rete WiFi

// Display ROSSI
#define RED_CLK 21    // CLK comune per tutti i display rossi
#define RED_TIME_DIO 22
#define RED_YEAR_DIO 25
#define RED_DAY_DIO 27

// Display ARANCIONI
#define AMBER_CLK 17    // CLK comune per tutti i display arancioni
#define AMBER_TIME_DIO 16
#define AMBER_YEAR_DIO 4
#define AMBER_DAY_DIO 19

// Display VERDI (nuovi)
#define GREEN_CLK 26    // CLK comune per tutti i display verdi
#define GREEN_TIME_DIO 18
#define GREEN_YEAR_DIO 23
#define GREEN_DAY_DIO 13

// Bus I2C condiviso per entrambi i display alfanumerici
#define SHARED_SDA 33
#define SHARED_SCL 32

// Indirizzi I2C
#define RED_MONTH_ADDR 0x70    // Nessun ponticello
#define AMBER_MONTH_ADDR 0x74  // A2 ponticellato
#define GREEN_MONTH_ADDR 0x72  // 0x70 + 2

// Definizione pin speaker
#define BUZZER_PIN 5

// Configurazione PWM per il buzzer
#define BUZZER_CHANNEL 0
#define BUZZER_FREQ 800    // Frequenza del beep in Hz
#define BUZZER_RESOLUTION 8

// Configurazione NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;     // Fuso orario (3600 = GMT+1)
const int   daylightOffset_sec = 0;    // Modificato: 0 perché l'ora legale è già gestita
unsigned long lastNtpSync = 0;
const unsigned long ntpSyncInterval = 3600000; // Sincronizzazione ogni ora (in millisecondi)

// Inizializzazione oggetti display ROSSI
TM1637Display displayRedTime(RED_CLK, RED_TIME_DIO);
TM1637Display displayRedYear(RED_CLK, RED_YEAR_DIO);
TM1637Display displayRedDay(RED_CLK, RED_DAY_DIO);
Adafruit_AlphaNum4 displayRedMonth = Adafruit_AlphaNum4();

// Inizializzazione oggetti display ARANCIONI
TM1637Display displayAmberTime(AMBER_CLK, AMBER_TIME_DIO);
TM1637Display displayAmberYear(AMBER_CLK, AMBER_YEAR_DIO);
TM1637Display displayAmberDay(AMBER_CLK, AMBER_DAY_DIO);
Adafruit_AlphaNum4 displayAmberMonth = Adafruit_AlphaNum4();

// Inizializzazione oggetti display VERDI
TM1637Display displayGreenTime(GREEN_CLK, GREEN_TIME_DIO);
TM1637Display displayGreenYear(GREEN_CLK, GREEN_YEAR_DIO);
TM1637Display displayGreenDay(GREEN_CLK, GREEN_DAY_DIO);
Adafruit_AlphaNum4 displayGreenMonth = Adafruit_AlphaNum4();

// Variabile per memorizzare il colore selezionato
String selectedColor = "red";  // Default: rosso

// Inizializzazione WebServer sulla porta 80
WebServer server(80);

// Variabili globali per il lampeggio dei due punti
unsigned long previousMillis = 0;
const long blinkInterval = 500;    // Intervallo di lampeggio
bool colonOn = false;              // Stato corrente dei due punti
String redTimeValue = "";         // Valore orario rosso
String amberTimeValue = "";       // Valore orario arancione
String greenTimeValue = "";       // Valore orario verde

// Aggiungiamo variabili per tenere traccia dell'ultimo minuto visualizzato
unsigned long lastMinuteUpdate = 0;
int lastMinuteDisplayed = -1;

// Pagina HTML con CSS e JavaScript integrati
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
    let selectedColor = 'red'; // Default: rosso
    let audioContext = null;

    // Frequenze DTMF per ogni tasto
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
        const duration = 0.1;  // durata del suono in secondi
        
        const osc1 = audioContext.createOscillator();
        const osc2 = audioContext.createOscillator();
        const gainNode = audioContext.createGain();
        
        gainNode.gain.value = 0.1;  // volume ridotto
        
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
              inputMode = 'month'; // Torna al mese per il prossimo ciclo
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
    
    // Inizializza la modalità di input
    updateInputMode();
  </script>
</body>
</html>
)rawliteral";

// Funzione per emettere un breve beep
void shortBeep() {
  tone(BUZZER_PIN, 2000);
  delay(10);               // Ridotto da 20ms a 10ms
  noTone(BUZZER_PIN);
}

// Funzione per animazione display numerico (per tempo, anno e giorno)
void animateNumber(TM1637Display &display, int finalNumber, int digits = 4, bool withColon = false) {
  int shuffleTime = 1000;  // Ripristinato a 1000ms 
  int updateInterval = 5;  // Manteniamo questo rapido per fluidità
  int beepInterval = 50;   // Nuovo: intervallo minimo tra i beep
  int startTime = millis();
  int lastBeepTime = 0;    // Nuovo: per tracciare l'ultimo beep
  
  // Array per tenere traccia di quali posizioni sono state "fissate"
  bool digitFixed[4] = {false, false, false, false};
  int finalDigits[4];
  
  // Estrai le cifre finali
  int temp = finalNumber;
  for(int i = digits-1; i >= 0; i--) {
    finalDigits[i] = temp % 10;
    temp /= 10;
  }
  
  // Fase di shuffle
  while(millis() - startTime < shuffleTime) {
    uint8_t segments[4] = {0, 0, 0, 0};
    
    for(int i = 0; i < digits; i++) {
      if(!digitFixed[i]) {
        // Nuovo: controlla l'intervallo tra i beep
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
        // Mantieni il numero finale per le posizioni già fissate
        if(digits == 4) {
          segments[i] = display.encodeDigit(finalDigits[i]);
        } else {
          segments[i+1] = display.encodeDigit(finalDigits[i]);
        }
      }
    }
    
    // Mostra i segmenti
    display.setSegments(segments);
    if(withColon && (millis() % 1000 < 500)) {
      display.showNumberDecEx(finalNumber, 0b01000000, true);
    }
    
    delay(updateInterval);
    
    // Fissa progressivamente le cifre da sinistra a destra
    int elapsed = millis() - startTime;
    for(int i = 0; i < digits; i++) {
      if(elapsed > (shuffleTime/digits * i) && !digitFixed[i]) {
        digitFixed[i] = true;
      }
    }
  }
  
  // Mostra il numero finale
  if(withColon) {
    display.showNumberDecEx(finalNumber, 0b01000000, true);
  } else if(digits == 4) {
    display.showNumberDec(finalNumber, false);
  } else {
    displayDayCentered(finalNumber, display);
  }
}

// Funzione per animazione display alfanumerico (mese)
void animateMonth(const char* monthStr, Adafruit_AlphaNum4 &display) {
  int shuffleTime = 1000;  // Ripristinato a 1000ms
  int updateInterval = 10;  // Manteniamo questo rapido per fluidità
  int startTime = millis();
  int len = strlen(monthStr);
  int startPos = 4 - len;  // Per allineamento a destra
  
  // Array per tenere traccia di quali posizioni sono state "fissate"
  bool charFixed[4] = {false, false, false, false};
  
  // Fase di shuffle
  while(millis() - startTime < shuffleTime) {
    // Genera caratteri casuali e emette beep
    for(int i = 0; i < len; i++) {
      if(!charFixed[i]) {
        shortBeep();  // Beep per ogni cambio di lettera
        display.writeDigitAscii(startPos + i, random(65, 90)); // Lettere maiuscole A-Z
      } else {
        display.writeDigitAscii(startPos + i, monthStr[i]);
      }
    }
    display.writeDisplay();
    delay(updateInterval);
    
    // Fissa progressivamente i caratteri da sinistra a destra
    int elapsed = millis() - startTime;
    for(int i = 0; i < len; i++) {
      if(elapsed > (shuffleTime/len * i) && !charFixed[i]) {
        charFixed[i] = true;
      }
    }
  }
  
  // Mostra il testo finale
  displayMonthRight(monthStr, display);
}

// Funzione per visualizzare il mese allineato a destra
void displayMonthRight(const char* monthStr, Adafruit_AlphaNum4 &display) {
  display.clear();
  int len = strlen(monthStr);
  int startPos = 4 - len;  // Calcola la posizione di partenza per l'allineamento a destra
  
  // Riempi con spazi le posizioni iniziali
  for(int i = 0; i < startPos; i++) {
    display.writeDigitAscii(i, ' ');
  }
  
  // Scrivi il mese partendo dalla posizione calcolata
  for(int i = 0; i < len && i < 4; i++) {
    display.writeDigitAscii(startPos + i, monthStr[i]);
  }
  
  display.writeDisplay();
}

// Funzione per visualizzare il giorno centrato nel display
void displayDayCentered(int day, TM1637Display &display) {
  uint8_t segments[] = { 0, 0, 0, 0 };  // Array per i 4 digit
  
  // Converti il giorno in due cifre
  int tens = day / 10;
  int ones = day % 10;
  
  // Posiziona le cifre al centro (posizioni 1 e 2)
  segments[1] = display.encodeDigit(tens);
  segments[2] = display.encodeDigit(ones);
  
  // Lascia vuote le posizioni esterne (0 e 3)
  segments[0] = 0;
  segments[3] = 0;
  
  // Visualizza sul display
  display.setSegments(segments);
}

// Funzione per animare tutti i display in sequenza
void animateAllDisplays(int year = 2024, int month = 1, int day = 31, int time = 1234) {
  // Valori predefiniti se non specificati
  const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
                         "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
  
  // Conversione mese in stringa
  const char* monthStr = months[month-1];
  
  // Sequenza ROSSA
  Serial.println("Animazione display ROSSI...");
  
  // Mese rosso
  animateMonth(monthStr, displayRedMonth);
  delay(200);  // Pausa tra animazioni
  
  // Giorno rosso
  animateNumber(displayRedDay, day, 2);
  delay(200);
  
  // Anno rosso
  animateNumber(displayRedYear, year);
  delay(200);
  
  // Orario rosso
  animateNumber(displayRedTime, time, 4, true);
  redTimeValue = String(time);
  delay(500);  // Pausa più lunga tra colori
  
  // Sequenza VERDE
  Serial.println("Animazione display VERDI...");
  
  // Mese verde
  animateMonth(monthStr, displayGreenMonth);
  delay(200);
  
  // Giorno verde
  animateNumber(displayGreenDay, day, 2);
  delay(200);
  
  // Anno verde
  animateNumber(displayGreenYear, year);
  delay(200);
  
  // Orario verde
  animateNumber(displayGreenTime, time, 4, true);
  greenTimeValue = String(time);
  delay(500);  // Pausa più lunga tra colori
  
  // Sequenza ARANCIONE
  Serial.println("Animazione display ARANCIONI...");
  
  // Mese arancione
  animateMonth(monthStr, displayAmberMonth);
  delay(200);
  
  // Giorno arancione
  animateNumber(displayAmberDay, day, 2);
  delay(200);
  
  // Anno arancione
  animateNumber(displayAmberYear, year);
  delay(200);
  
  // Orario arancione
  animateNumber(displayAmberTime, time, 4, true);
  amberTimeValue = String(time);
  
  // Reset delle variabili per il lampeggio
  previousMillis = 0;
  colonOn = true;
  
  Serial.println("Sequenza completa!");
}

// Funzione per aggiornare il lampeggio dei due punti su tutti i display
void updateTimeWithBlink() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;
    colonOn = !colonOn;  // Inverte lo stato dei due punti
    
    // Aggiorna tutti i display dell'orario con i due punti
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

// Funzione per aggiornare i display verdi con l'orario NTP
void updateGreenDisplaysFromNTP() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Impossibile ottenere l'ora da NTP");
    return;
  }
  
  Serial.println("Ora ottenuta dal server NTP");
  
  // Formatta l'ora per il display (HHMM)
  int hours = timeinfo.tm_hour;
  int minutes = timeinfo.tm_min;
  int timeValue = hours * 100 + minutes;
  
  // Formatta la data
  int year = timeinfo.tm_year + 1900;
  int month = timeinfo.tm_mon + 1;
  int day = timeinfo.tm_mday;
  
  // Array dei nomi dei mesi
  const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
                         "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
  
  // Aggiorna i display verdi con animazione
  animateMonth(months[month-1], displayGreenMonth);
  delay(200);
  animateNumber(displayGreenDay, day, 2);
  delay(200);
  animateNumber(displayGreenYear, year);
  delay(200);
  animateNumber(displayGreenTime, timeValue, 4, true);
  greenTimeValue = String(timeValue);
  
  Serial.println("Display verdi aggiornati con l'ora NTP");
}

// Funzione per aggiornare la luminosità in base all'ora
void updateBrightness() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    // Calcola l'ora e i minuti come un valore numerico per facilitare il confronto
    int currentTime = timeinfo.tm_hour * 100 + timeinfo.tm_min;
    
    // Luminosità alta (100%) dalle 8:00 (800) alle 21:30 (2130)
    // Luminosità bassa (65%) dalle 21:31 (2131) alle 7:59 (759)
    bool isHighBrightness = (currentTime >= 800 && currentTime <= 2130);
    
    // Imposta la luminosità per tutti i display
    int tm1637Brightness = isHighBrightness ? 7 : 4;  // TM1637: 7 = 100%, 4 = ~57%
    int ht16k33Brightness = isHighBrightness ? 15 : 10; // HT16K33: 15 = 100%, 10 = ~67%
    
    // Display ROSSI
    displayRedTime.setBrightness(tm1637Brightness);
    displayRedYear.setBrightness(tm1637Brightness);
    displayRedDay.setBrightness(tm1637Brightness);
    displayRedMonth.setBrightness(ht16k33Brightness);
    
    // Display VERDI
    displayGreenTime.setBrightness(tm1637Brightness);
    displayGreenYear.setBrightness(tm1637Brightness);
    displayGreenDay.setBrightness(tm1637Brightness);
    displayGreenMonth.setBrightness(ht16k33Brightness);
    
    // Display ARANCIONI
    displayAmberTime.setBrightness(tm1637Brightness);
    displayAmberYear.setBrightness(tm1637Brightness);
    displayAmberDay.setBrightness(tm1637Brightness);
    displayAmberMonth.setBrightness(ht16k33Brightness);
  }
}

void setup() {
  Serial.begin(115200);
  
  // Inizializzazione I2C per i display alfanumerici
  Wire.begin(SHARED_SDA, SHARED_SCL);
  
  // Inizializzazione display ROSSI
  displayRedTime.setBrightness(5);
  displayRedTime.clear();
  
  displayRedYear.setBrightness(5);
  displayRedYear.clear();
  
  displayRedDay.setBrightness(5);
  displayRedDay.clear();
  
  displayRedMonth.begin(RED_MONTH_ADDR);
  displayRedMonth.setBrightness(15);
  displayRedMonth.clear();
  
  // Inizializzazione display ARANCIONI
  displayAmberTime.setBrightness(5);
  displayAmberTime.clear();
  
  displayAmberYear.setBrightness(5);
  displayAmberYear.clear();
  
  displayAmberDay.setBrightness(5);
  displayAmberDay.clear();
  
  displayAmberMonth.begin(AMBER_MONTH_ADDR);
  displayAmberMonth.setBrightness(15);
  displayAmberMonth.clear();
  
  // Inizializzazione display VERDI
  displayGreenTime.setBrightness(5);
  displayGreenTime.clear();
  
  displayGreenYear.setBrightness(5);
  displayGreenYear.clear();
  
  displayGreenDay.setBrightness(5);
  displayGreenDay.clear();
  
  displayGreenMonth.begin(GREEN_MONTH_ADDR);
  displayGreenMonth.setBrightness(15);
  displayGreenMonth.clear();
  
  // Configurazione pin buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Connessione WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connesso a WiFi. Indirizzo IP: ");
  Serial.println(WiFi.localIP());
  
  // Avvio del server e configurazione dei vari endpoint
  server.on("/", HTTP_GET, []() {
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
      redTimeValue = timeStr;  // Memorizza il valore per il lampeggio
      animateNumber(displayRedTime, time, 4, true);
    } else if(color == "green") {
      greenTimeValue = timeStr;  // Memorizza il valore per il lampeggio
      animateNumber(displayGreenTime, time, 4, true);
    } else { // amber
      amberTimeValue = timeStr;  // Memorizza il valore per il lampeggio
      animateNumber(displayAmberTime, time, 4, true);
    }
    server.send(200, "text/plain", "OK");
  });
  
  server.on("/animateAll", HTTP_GET, []() {
    int year = server.arg("year").toInt() > 0 ? server.arg("year").toInt() : 2024;
    int month = server.arg("month").toInt() > 0 ? server.arg("month").toInt() : 1;
    int day = server.arg("day").toInt() > 0 ? server.arg("day").toInt() : 1;
    int time = server.arg("time").toInt() >= 0 ? server.arg("time").toInt() : 0;
    
    // Avvia l'animazione
    animateAllDisplays(year, month, day, time);
    
    server.send(200, "text/plain", "Animazione completata!");
  });
  
  server.begin();
  
  // Esegui l'animazione completa all'avvio (UNA SOLA VOLTA)
  animateAllDisplays(2024, 1, 1, 0000);
  
  // Pulisci tutte le variabili
  redTimeValue = "";
  amberTimeValue = "";
  greenTimeValue = "";
  
  // Dopo la connessione WiFi
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("Connesso a WiFi. Indirizzo IP: ");
    Serial.println(WiFi.localIP());
    
    // Configura il client NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    // Sincronizza e aggiorna i display verdi
    updateGreenDisplaysFromNTP();
  }
}

void loop() {
  server.handleClient();
  updateTimeWithBlink();  // Aggiorna il lampeggio per tutti i display
  
  // Ottieni l'ora corrente
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    // Aggiornamento minutario 
    if (timeinfo.tm_min != lastMinuteDisplayed) {
      // Formatta l'ora per il display (HHMM)
      int hours = timeinfo.tm_hour;
      int minutes = timeinfo.tm_min;
      int timeValue = hours * 100 + minutes;
      
      // Aggiorna solo il display verde dell'ora, senza animazione
      displayGreenTime.showNumberDecEx(timeValue, 0b01000000, true);
      greenTimeValue = String(timeValue);
      
      // Se cambiano ora, giorno, mese o anno, aggiorna anche quelli
      static int lastDay = -1, lastMonth = -1, lastYear = -1;
      
      if (timeinfo.tm_mday != lastDay) {
        displayDayCentered(timeinfo.tm_mday, displayGreenDay);
        lastDay = timeinfo.tm_mday;
      }
      
      if (timeinfo.tm_mon != lastMonth) {
        const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
                               "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
        displayMonthRight(months[timeinfo.tm_mon], displayGreenMonth);
        lastMonth = timeinfo.tm_mon;
      }
      
      if (timeinfo.tm_year != lastYear) {
        int year = timeinfo.tm_year + 1900;
        displayGreenYear.showNumberDec(year);
        lastYear = timeinfo.tm_year;
      }
      
      // Aggiorna la luminosità quando cambia il minuto
      updateBrightness();
      
      // Controllo per la sincronizzazione all'inizio dell'ora
      if (timeinfo.tm_min == 0) {  // Se siamo all'inizio dell'ora (XX:00)
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("Sincronizzazione NTP all'inizio dell'ora");
          updateGreenDisplaysFromNTP();
        }
      }
      
      lastMinuteDisplayed = timeinfo.tm_min;
    }
  }
}
