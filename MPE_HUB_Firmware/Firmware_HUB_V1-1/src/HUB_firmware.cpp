#include <HUB_firmware.h>
#include "ESPTelnetStream.h"
#include "driver/gpio.h"
#include <log.h>

/*--------------ISTANZE--------------*/
ESPTelnetStream telnet;
uint16_t  port = 23;
Adafruit_ADS1015 ads;

// Mappatura porte seriali per 485 dedicati
HardwareSerial SerialROV(0);
HardwareSerial SerialLampSX(1);
HardwareSerial SerialLampDX(2);

// Dichiarazione della struct contenente le variabili di sistema
systemStatusStruct systemStatus;

// Creo un'istanza del logger per scrivere e leggere sul file "log.txt"
Logger logger("/log.txt");

const char* WarningNames[WARNING_LAST_INDEX] = {      // stringhe associate ai nomi dei warning
    "UNDERVOLTAGE_12V",
    "OVERVOLTAGE_12V",
    "OVERTEMP_12V",
    "UNDERVOLTAGE_24V",
    "OVERVOLTAGE_24V",
    "OVERTEMP_24V",
    "OVERCURRENT_IPCAM",
    "OVERCURRENT_BD3D",
    "OVERCURRENT_LAMPSX",
    "OVERCURRENT_LAMPDX",
    "LAMPSX_DISCONNECTED",
    "LAMPDX_DISCONNECTED"
};

const char* ErrorNames[ERROR_LAST_INDEX] = {          // stringhe associate ai nomi degli errori
    "UNDERVOLTAGE_12V",
    "OVERVOLTAGE_12V",
    "OVERTEMP_12V",
    "UNDERVOLTAGE_24V",
    "OVERVOLTAGE_24V",
    "OVERTEMP_24V",
    "OVERCURRENT_IPCAM",
    "OVERCURRENT_BD3D",
    "OVERCURRENT_LAMPSX",
    "OVERCURRENT_LAMPDX",
    "WATER_LEAK_DETECTED"
};

// TODO aggiungere funzione per avviare una procedura di spegnimento in caso di errori

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
  EEPROM.readBytes(ADDR_ID_NUM, systemStatus.ID, ID_NUM_SIZE);                          // Legge l'ID salvato nella EEPROM
  if (systemStatus.ID[0] == 'H' && systemStatus.ID[1] == 'B' && systemStatus.ID[2] == 'S' && systemStatus.ID[3] == 'W') {   // Se l'ID è valido allora lo stampa
    writeTelnet("Device ID: " + String(systemStatus.ID));                               // Stampa l'ID del dispositivo
    write485ROV("HUB Device ID: " + String(systemStatus.ID) + "\n\r");                  // Invia l'ID del dispositivo al ROV
  } else {
    writeTelnet("Device ID not valid");
  }

  /*
  // COMANDO PER RESETTARE IL CONTATORE DEI CICLI DI ACCENSIONE 
  EEPROM.writeByte(ADDR_FIRST_CYCLE,0);
  EEPROM.commit();
  */

  if (EEPROM.readByte(ADDR_FIRST_CYCLE) != FIRST_CYCLE_KEY) {     // Controlla se è il primo avvio
    EEPROM.writeByte(ADDR_FIRST_CYCLE, FIRST_CYCLE_KEY);          // Scrive la chiave per indicare che non è più il primo avvio
    EEPROM.put(ADDR_POWER_CYCLE, systemStatus.power_cycle_count); // Inizializza il contatore dei cicli di accensione a 0
    EEPROM.commit();                                              // Salva le modifiche nella EEPROM
  } else {  // Non è il primo avvio, incrementa il contatore dei cicli di accensione
    EEPROM.get(ADDR_POWER_CYCLE, systemStatus.power_cycle_count);            // Legge il contatore dei cicli di accensione
    if (systemStatus.power_cycle_count < 65535) {                            // Evita overflow del contatore
      systemStatus.power_cycle_count++;
      EEPROM.put(ADDR_POWER_CYCLE, systemStatus.power_cycle_count);          // Aggiorna il contatore
      EEPROM.commit();                                            // Salva le modifiche nella EEPROM
    }
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

  // ------------ Filesystem --------------
  if(!logger.begin(false)) {                // Prova a montare il filesystem
    write485ROV("HUB Mount failed\n\r");    // Se fallisce, messaggio di errore
    return;
  }

  #ifdef LOG_DEBUG
    logger.checkFS(telnet);
  #endif

  // FIXME implementare funzione di logging iniziale
  //logger.log("\n\r# " + String(systemStatus.power_cycle_count) + "\n\r"); // Scrive nel log il numero di avvi della scheda
  // TODO aggiungere logging periodico e in caso di errore

  tone(BUZZER_DEBUG, 300, 100);
}

void systemStatusCheck(){
  // Resetta tutti gli elementi a false (0)
  memset(systemStatus.WarningFlags, 0, sizeof(systemStatus.WarningFlags));
  memset(systemStatus.ErrorFlags, 0, sizeof(systemStatus.ErrorFlags));


  // Check Overtemp
  if (systemStatus.ic12V_temperature > WARNING_OVERTEMP_12V_THRESHOLD){
    systemStatus.WarningFlags[WARNING_OVERTEMP_12V] = true;
  } else systemStatus.WarningFlags[WARNING_OVERTEMP_12V] = false;
  
  if (systemStatus.ic12V_temperature > ERROR_OVERTEMP_12V_THRESHOLD){
    systemStatus.ErrorFlags[ERROR_OVERTEMP_12V] = true;
  } else systemStatus.ErrorFlags[ERROR_OVERTEMP_12V] = false;
  
  if (systemStatus.ic24V_temperature > WARNING_OVERTEMP_24V_THRESHOLD){
    systemStatus.WarningFlags[WARNING_OVERTEMP_24V] = true;
  } else systemStatus.WarningFlags[WARNING_OVERTEMP_24V] = false;
  
  if (systemStatus.ic24V_temperature > ERROR_OVERTEMP_24V_THRESHOLD){
    systemStatus.ErrorFlags[ERROR_OVERTEMP_24V] = true;
  } else systemStatus.ErrorFlags[ERROR_OVERTEMP_24V] = false;


  // Check Undervoltage
  if (systemStatus.ic12V_voltage < WARNING_UNDERVOLTAGE_12V_THRESHOLD){
    systemStatus.WarningFlags[WARNING_UNDERVOLTAGE_12V] = true;
  } else systemStatus.WarningFlags[WARNING_UNDERVOLTAGE_12V] = false;
  
  if (systemStatus.ic12V_voltage < ERROR_UNDERVOLTAGE_12V_THRESHOLD){
    systemStatus.ErrorFlags[ERROR_UNDERVOLTAGE_12V] = true;
  } else systemStatus.ErrorFlags[ERROR_UNDERVOLTAGE_12V] = false;
  
  if (systemStatus.ic24V_voltage < WARNING_UNDERVOLTAGE_24V_THRESHOLD){
    systemStatus.WarningFlags[WARNING_UNDERVOLTAGE_24V] = true;
  } else systemStatus.WarningFlags[WARNING_UNDERVOLTAGE_24V] = false;
  
  if (systemStatus.ic24V_voltage < ERROR_UNDERVOLTAGE_24V_THRESHOLD){
    systemStatus.ErrorFlags[ERROR_UNDERVOLTAGE_24V] = true;
  } else systemStatus.ErrorFlags[ERROR_UNDERVOLTAGE_24V] = false;
  

  // Check Overvoltage
  if (systemStatus.ic12V_voltage > WARNING_OVERVOLTAGE_12V_THRESHOLD){
    systemStatus.WarningFlags[WARNING_OVERVOLTAGE_12V] = true;
  } else systemStatus.WarningFlags[WARNING_OVERVOLTAGE_12V] = false;

  if (systemStatus.ic12V_voltage > ERROR_OVERVOLTAGE_12V_THRESHOLD){
    systemStatus.ErrorFlags[ERROR_OVERVOLTAGE_12V] = true;
  } else systemStatus.ErrorFlags[ERROR_OVERVOLTAGE_12V] = false;

  if (systemStatus.ic24V_voltage > WARNING_OVERVOLTAGE_24V_THRESHOLD){
    systemStatus.WarningFlags[WARNING_OVERVOLTAGE_24V] = true;
  } else systemStatus.WarningFlags[WARNING_OVERVOLTAGE_24V] = false;

  if (systemStatus.ic24V_voltage > ERROR_OVERVOLTAGE_24V_THRESHOLD){
    systemStatus.ErrorFlags[ERROR_OVERVOLTAGE_24V] = true;
  } else systemStatus.ErrorFlags[ERROR_OVERVOLTAGE_24V] = false;


  // Check Overcurrent
  if (systemStatus.ipcam_current > WARNING_OVERCURRENT_IPCAM_THRESHOLD){
    systemStatus.WarningFlags[WARNING_OVERCURRENT_IPCAM] = true;
  } else systemStatus.WarningFlags[WARNING_OVERCURRENT_IPCAM] = false;
  
  if (systemStatus.ipcam_current > ERROR_OVERCURRENT_IPCAM_THRESHOLD){
    systemStatus.ErrorFlags[ERROR_OVERCURRENT_IPCAM] = true;
  } else systemStatus.ErrorFlags[ERROR_OVERCURRENT_IPCAM] = false;
  
  if (systemStatus.bd3d_current > WARNING_OVERCURRENT_BD3D_THRESHOLD){
    systemStatus.WarningFlags[WARNING_OVERCURRENT_BD3D] = true;
  } else systemStatus.WarningFlags[WARNING_OVERCURRENT_BD3D] = false;
  
  if (systemStatus.bd3d_current > ERROR_OVERCURRENT_BD3D_THRESHOLD){
    systemStatus.ErrorFlags[ERROR_OVERCURRENT_BD3D] = true;
  } else systemStatus.ErrorFlags[ERROR_OVERCURRENT_BD3D] = false;
  
  if (systemStatus.lamp1_current > WARNING_OVERCURRENT_LAMPSX_THRESHOLD){
    systemStatus.WarningFlags[WARNING_OVERCURRENT_LAMPSX] = true;
  } else systemStatus.WarningFlags[WARNING_OVERCURRENT_LAMPSX] = false;
  
  if (systemStatus.lamp1_current > ERROR_OVERCURRENT_LAMPSX_THRESHOLD){
    systemStatus.ErrorFlags[ERROR_OVERCURRENT_LAMPSX] = true;
  } else systemStatus.ErrorFlags[ERROR_OVERCURRENT_LAMPSX] = false;
  
  if (systemStatus.lamp2_current > WARNING_OVERCURRENT_LAMPDX_THRESHOLD){
    systemStatus.WarningFlags[WARNING_OVERCURRENT_LAMPDX] = true;
  } else systemStatus.WarningFlags[WARNING_OVERCURRENT_LAMPDX] = false;
  
  if (systemStatus.lamp2_current > ERROR_OVERCURRENT_LAMPDX_THRESHOLD){
    systemStatus.ErrorFlags[ERROR_OVERCURRENT_LAMPDX] = true;
  } else systemStatus.ErrorFlags[ERROR_OVERCURRENT_LAMPDX] = false;


  // Verifico che la lampada SX rispondano ogni "LAMP_COMMUNICATION_TIMEOUT" millisecondi (se la lampada è accesa)
  if (millis() - systemStatus.last_lampSX_comm_time > LAMP_COMMUNICATION_TIMEOUT && systemStatus.ic24V_C0_state) {
    systemStatus.WarningFlags[WARNING_LAMPSX_DISCONNECTED] = true;
  }
  else systemStatus.WarningFlags[WARNING_LAMPSX_DISCONNECTED] = false;


  // Verifico che la lampada DX rispondano ogni "LAMP_COMMUNICATION_TIMEOUT" millisecondi (se la lampada è accesa)
  if (millis() - systemStatus.last_lampDX_comm_time > LAMP_COMMUNICATION_TIMEOUT && systemStatus.ic24V_C1_state) {
    systemStatus.WarningFlags[WARNING_LAMPDX_DISCONNECTED] = true;
  } else systemStatus.WarningFlags[WARNING_LAMPDX_DISCONNECTED] = false;


  // Water leak detection
  if (digitalRead(WATER_PROBE_PIN) == HIGH){
    systemStatus.ErrorFlags[ERROR_WATER_LEAK_DETECTED] = true;
  } else systemStatus.ErrorFlags[ERROR_WATER_LEAK_DETECTED] = false;

  #ifdef DEBUG_STATUS
    // Stampa a terminale lo stato dei warning e degli errori ogni "PRINT_ERROR_TIMER" millisecondi
    if (millis() - systemStatus.last_error_print_time > PRINT_ERROR_TIMER){
      printSystemStatus();
      systemStatus.last_error_print_time = millis();
    }
  #endif

}

void printSystemStatus() {
    writeTelnet("\n\r----- SYSTEM STATUS -----");

    writeTelnet("ID:" + String(systemStatus.ID) + "\r");

    writeTelnet("Power cycle count:" + String(systemStatus.power_cycle_count));

    writeTelnet("OTA progress (ms):" + String(systemStatus.ota_progress_millis));

    writeTelnet("Last Lamp SX comm time:" + String(systemStatus.last_lampSX_comm_time));
    writeTelnet("Last Lamp DX comm time:" + String(systemStatus.last_lampDX_comm_time));

    writeTelnet("Lamp1 current:" + String(systemStatus.lamp1_current));
    writeTelnet("Lamp2 current:" + String(systemStatus.lamp2_current));

    writeTelnet("IC 24V voltage:" + String(systemStatus.ic24V_voltage));
    writeTelnet("IC 24V temperature:" + String(systemStatus.ic24V_temperature));

    writeTelnet("IC 24V C0 state:" + String(systemStatus.ic24V_C0_state ? "ON" : "OFF"));
    writeTelnet("IC 24V C1 state:" + String(systemStatus.ic24V_C1_state ? "ON" : "OFF"));

    writeTelnet("BD3D current:" + String(systemStatus.bd3d_current));
    writeTelnet("IPCam current:" + String(systemStatus.ipcam_current));

    writeTelnet("IC 12V voltage:" + String(systemStatus.ic12V_voltage));
    writeTelnet("IC 12V temperature:" + String(systemStatus.ic12V_temperature));

    writeTelnet("IC 12V C0 state:" + String(systemStatus.ic12V_C0_state ? "ON" : "OFF"));
    writeTelnet("IC 12V C1 state:" + String(systemStatus.ic12V_C1_state ? "ON" : "OFF"));

    writeTelnet("ID print flag:" + String(systemStatus.id_print_flag ? "TRUE" : "FALSE"));
    writeTelnet("Lamp reset flag:" + String(systemStatus.lamp_reset_flag ? "TRUE" : "FALSE"));

    writeTelnet("Warnings:");
    for (int i = 0; i < WARNING_LAST_INDEX; i++) {
        writeTelnet(String("  " + String(WarningNames[i]) + ": " + (systemStatus.WarningFlags[i] ? "TRUE" : "FALSE")).c_str());
    }

    writeTelnet("Errors:");
    for (int i = 0; i < ERROR_LAST_INDEX; i++) {
        writeTelnet(String("  " + String(ErrorNames[i]) + ": " + (systemStatus.ErrorFlags[i] ? "TRUE" : "FALSE")).c_str());
    }

    #ifdef LOG_DEBUG
      writeTelnet("--------------------------\n\r");
      logger.readAll(telnet);   // Invia il contenuto del log alla telnet
      writeTelnet("-------++++++++++++-------\n\r");
      logger.checkFS(telnet);   // Stampo le info della memoria
    #endif
}


// FUNZIONE PER CONVERTIRE VELOCEMENTE LA LETTURA ANALOGICA DI UN PIN DALL'ADC [V]
float getAnalogueVoltage(uint8_t pin_number){
  int16_t voltage_reading = ads.readADC_SingleEnded(pin_number);
  float voltage_divider = (float)(R2_ADC_DIVIDER + R1_ADC_DIVIDER)/(float)(R2_ADC_DIVIDER);
  float current_divider = (float)(R1_ADC_DIVIDER + R2_ADC_DIVIDER + RC_ADC_DIVIDER)/(float)(R1_ADC_DIVIDER + R2_ADC_DIVIDER);
  float voltage_conversion = ads.computeVolts(voltage_reading)*voltage_divider*current_divider;
  return voltage_conversion;
}

// FUNZIONE CHE STAMPA TUTTE LE TENSIONI LETTE DALL'ADC
void print_ADC(){
  writeTelnet("ADC Voltage reading: ");
  writeTelnet(String(ads.readADC_SingleEnded(MULTISENSE_12V_ADC_PIN)) + " RAW -> Multisense 12V \t" + String(getAnalogueVoltage(MULTISENSE_12V_ADC_PIN)) + " Volt -> Multisense 12V");
  writeTelnet(String(ads.readADC_SingleEnded(MULTISENSE_24V_ADC_PIN)) + " RAW -> Multisense 24V \t" + String(getAnalogueVoltage(MULTISENSE_24V_ADC_PIN)) + " Volt -> Multisense 24V");
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

String getHTMLpage(){
  String html_String = "";
  File file = LittleFS.open("/index.html", "r");
  if(!file){
    writeTelnet("Failed to open index.html");
  }
  while(file.available()){
      html_String += char(file.read());
  }
  file.close();
  return html_String;
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