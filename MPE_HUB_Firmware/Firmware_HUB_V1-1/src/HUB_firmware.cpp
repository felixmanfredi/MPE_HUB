#include <HUB_firmware.h>
#include "ESPTelnetStream.h"
#include "driver/gpio.h"

/*--------------ISTANZE--------------*/
ESPTelnetStream telnet;
uint16_t  port = 23;
Adafruit_ADS1015 ads;

// Mappatura porte seriali per 485 dedicati
HardwareSerial SerialROV(0);
HardwareSerial SerialLampSX(1);
HardwareSerial SerialLampDX(2);

/*--------------FUNZIONI------------*/

// FUNZIONE PER LA DICHIARAZIONE DI PIN CONTENUTI IN UN ARRAY
void declaration_function(const uint8_t array[], uint8_t size,  byte type){
    int index_variable = 0;
    for (index_variable=0; index_variable< size; index_variable++){
        pinMode(array[index_variable], type);
    }
}

// FUNZIONE PER IMPOSTARE IL VALORE DEI PIN CONTENUTI IN UN ARRAY
void set_pin_function(const uint8_t array[], uint8_t size, byte value){
    int index_variable = 0;
    for (index_variable=0; index_variable< size; index_variable++){
        digitalWrite(array[index_variable], value);
    }
}

// FUNZIONE PER L'INIZIALIZZAZIONE DI TUTTE LE ISTANZE
void initialize(){
  // Inizializzazione PWM per PWM_LIGHT
  ledcSetup(0, 5000, 8); // Canale 0, frequenza 5kHz, risoluzione 8 bit
  ledcAttachPin(PWM_LIGHT, 0); // Associa PWM_LIGHT al canale 0

  declaration_function(OUTPUT_ARRAY, sizeof(OUTPUT_ARRAY), OUTPUT);
  set_pin_function(OUTPUT_ARRAY, sizeof(OUTPUT_ARRAY), LOW);
  declaration_function(INPUT_ARRAY, sizeof(INPUT_ARRAY), INPUT);

  //Serial.begin(SERIAL_SPEED);
  SerialROV.begin(SERIAL_SPEED, SERIAL_8N1, RX_485_ROV, TX_485_ROV);          // begin RS485 ROV
  SerialLampSX.begin(SERIAL_SPEED, SERIAL_8N1, RX_485_LED_SX, TX_485_LED_SX);  // begin RS485 LED SX
  SerialLampDX.begin(SERIAL_SPEED, SERIAL_8N1, RX_485_LED_DX, TX_485_LED_DX);  // begin RS485 LED DX

  SerialROV.setTimeout(100);    // imposto il timeout del readStringUnitil per evitare che resti bloccato troppo a lungo
  SerialLampSX.setTimeout(100); // imposto il timeout del readStringUnitil per evitare che resti bloccato troppo a lungo
  SerialLampDX.setTimeout(100); // imposto il timeout del readStringUnitil per evitare che resti bloccato troppo a lungo

  Wire.begin(SDA_PIN, SCL_PIN);                   // begin i2c come master
  delay(10);

  //Serial.println(psramFound() ? "PSRAM Abilitata" : "PSRAM Disabilitata");

  /* Setup EEPROM */
  EEPROM.begin(EEPROM_SIZE);                      // inizializzo la memoria da utilizzare
  /* Stampa ID */
  char id_num[ID_NUM_SIZE];
  EEPROM.readBytes(ADDR_ID_NUM, id_num, ID_NUM_SIZE);                           // Legge l'ID salvato nella EEPROM
  if (isLetter(id_num[0]) & isLetter(id_num[1]) & isLetter(id_num[2])) {        // Se l'ID è valido allora lo stampa
    writeTelnet("Device ID: " + String(id_num));                                // Stampa l'ID del dispositivo
  } else {
    writeTelnet("Device ID not valid");
  }

  /* Setup ADC */
  if (!ads.begin(ADC_ADDR, &Wire)) {
    writeTelnet("Failed to initialize ADS.\n\r");
  } else {
    writeTelnet("ADS initialized successfully.\n\r");
    ads.setGain(GAIN_ONE);
  }

  pinMode(RST_GPIO, OUTPUT);
  digitalWrite(RST_GPIO, HIGH);
  
  tone(BUZZER_DEBUG, 300, 100);
}

// FUNZIONE PER CONVERTIRE VELOCEMENTE LA LETTURA ANALOGICA DI UN PIN DALL'ADC [V]
float getAnalogueVoltage(uint8_t pin_number){
  int16_t voltage_reading = ads.readADC_SingleEnded(pin_number);
  float divider = (R2_ADC_DIVIDER + R1_ADC_DIVIDER)/(R2_ADC_DIVIDER);
  float voltage_conversion = ads.computeVolts(voltage_reading)*divider;
  return voltage_conversion;
}

// FUNZIONE CHE STAMPA TUTTE LE TENSIONI LETTE DALL'ADC
void print_ADC(){
  writeTelnet("ADC Voltage reading: ");
  writeTelnet(String(ads.readADC_SingleEnded(MULTISENSE_12V_ADC_PIN)) + " RAW -> DCDC internal \t" + String(getAnalogueVoltage(MULTISENSE_12V_ADC_PIN)) + " Volt -> DCDC internal");
  writeTelnet(String(ads.readADC_SingleEnded(MULTISENSE_24V_ADC_PIN)) + " RAW -> DCDC out \t" + String(getAnalogueVoltage(MULTISENSE_24V_ADC_PIN)) + " Volt -> DCDC out");
}

// FUNZIONE CHE STAMPA IL TESTO SUL 485 DEDICATO AL ROV
void write485ROV(String text){
    digitalWrite(RW_485_ROV,HIGH);                  // attivazione scrittura 485
    SerialROV.print(text);                          // stampa il testo in 485
    delay(10);                                      // attesa per completamento trasmissione
    digitalWrite(RW_485_ROV,LOW);                   // attivazione lettura 485
}

// FUNZIONE CHE STAMPA IL TESTO SUL 485 DEDICATO ALLA LAMPADA SX
void write485LampSX(String text){
    digitalWrite(RW_485_LED_SX,HIGH);               // attivazione scrittura 485
    SerialLampSX.print(text);                       // stampa il testo in 485
    delay(10);                                      // attesa per completamento trasmissione
    digitalWrite(RW_485_LED_SX,LOW);                // attivazione lettura 485
}

// FUNZIONE CHE STAMPA IL TESTO SUL 485 DEDICATO ALLA LAMPADA DX
void write485LampDX(String text){
    digitalWrite(RW_485_LED_DX,HIGH);               // attivazione scrittura 485
    SerialLampDX.print(text);                       // stampa il testo in 485
    delay(10);                                      // attesa per completamento trasmissione
    digitalWrite(RW_485_LED_DX,LOW);                // attivazione lettura 485
}

// FUNZIONE PER SCANSIONARE TUTTI I DISPOSITIVI SUL BUS
void scanI2C(){
  byte error, address;
  int nDevices;
  writeTelnet("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      writeTelnet("I2C device found at address 0x");
      if (address<16) {
        writeTelnet("0");
      }
      writeTelnet(String(address,HEX));
      nDevices++;
    }
    else if (error==4) {
      writeTelnet("Unknow error at address 0x");
      if (address<16) {
        writeTelnet("0");
      }
      writeTelnet(String(address,HEX));
    }    
  }
  if (nDevices == 0) {
    writeTelnet("No I2C devices found\n");
  }
  else {
    writeTelnet("done\n");
  }
}

// FUNZIONE CHE VERIFICA SE UN CARATTERE è UNA LETTERA
bool isLetter(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

// FUNZIONE CHE INOLTRA IL COMANDO DI RESET ALLE LAMPADE TRAMITE 485
void resetLamp(){
  write485LampSX("reset\n\r");                        // Invio del comando di reset ai flash tramite 485
  write485LampDX("reset\n\r");                        // Invio del comando di reset ai flash tramite 485
  write485ROV("Lamp reset sent\n\r");                 // Invio del comando di reset effettuato al ROV tramite 485
  writeTelnet("Lamp reset sent");                     // Stampa il comando di reset effettuato sulla telnet
}

void blinkDebugLED(uint8_t pin){
  digitalWrite(pin, HIGH);
  delay(BLINK_DELAY_TIME);
  digitalWrite(pin, LOW);
}

/*---------------TELNET-------------*/

bool isConnected() {
  return (WiFi.status() == WL_CONNECTED);
}

bool connectToWiFi(const char* ssid, const char* password, int max_tries = 20, int pause = 500) {
  int i = 0;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  #if defined(ARDUINO_ARCH_ESP8266)
    WiFi.forceSleepWake();
    delay(200);
  #endif
  WiFi.begin(ssid, password);
  do {
    delay(pause);
    //Serial.print(".");
    i++;
  } while (!isConnected() && i < max_tries);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  return isConnected();
}

void errorMsg(String error, bool restart = true) {
  //Serial.println(error);
  if (restart) {
    //Serial.println("Rebooting now...");
    delay(2000);
    ESP.restart();
    delay(2000);
  }
}

// (optional) callback functions for telnet events
void onTelnetConnect(String ip) {
  //Serial.print("- Telnet: ");
  //Serial.print(ip);
  //Serial.println(" connected");
  
  telnet.println("\nWelcome " + telnet.getIP());
  telnet.println("(Use ^] + q  to disconnect.)");
}

/* ------------------------------------------------- */

void onTelnetDisconnect(String ip) {
  //Serial.print("- Telnet: ");
  //Serial.print(ip);
  //Serial.println(" disconnected");
}

/* ------------------------------------------------- */

void onTelnetReconnect(String ip) {
  //Serial.print("- Telnet: ");
  //Serial.print(ip);
  //Serial.println(" reconnected");
}

/* ------------------------------------------------- */

void onTelnetConnectionAttempt(String ip) {
  //Serial.print("- Telnet: ");
  //Serial.print(ip);
  //Serial.println(" tried to connected");
}

/* ------------------------------------------------- */

void onTelnetInput(String str) {
  // checks for a certain command
  if (str == "ping") {
    telnet.println("> pong"); 
    //Serial.println("- Telnet: pong");
  // disconnect the client
  } else if (str == "bye") {
    telnet.println("> disconnecting you...");
    telnet.disconnectClient();
  } else {
    telnet.println(str);
  }
}

void setupTelnet() {  
  // passing on functions for various telnet events
  telnet.onConnect(onTelnetConnect);
  telnet.onConnectionAttempt(onTelnetConnectionAttempt);
  telnet.onReconnect(onTelnetReconnect);
  telnet.onDisconnect(onTelnetDisconnect);
  telnet.onInputReceived(onTelnetInput);

  //Serial.print("- Telnet: ");
  if (telnet.begin(port)) {
    //Serial.println("running");
  } else {
    //Serial.println("error.");
    errorMsg("Will reboot...");
  }

}

// FUNZIONE CON IL LOOP DI TELNET E VERIFICA SE CI SONO VALORI DA LEGGERE
String loopTelnet() {
    telnet.loop();      // NELLA FUNZIONE LOOP HO COMMENTATO "handleClientInput();" PERCHè SVUOTAVA IL BUFFER
    String telnetString = "";
    if (telnet.available() > 0){
        telnetString = telnet.readStringUntil('\n');
        telnet.print("# " + telnetString + "\n\r");
    }
    return telnetString;
}

// FUNZIONE CHE SCRIVE SU TELNET UNA STRINGA
void writeTelnet(String text){
  blinkDebugLED(LED_DEBUG_GREEN);
  telnet.println(text);
}