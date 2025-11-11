#include <HUB_firmware.h>
#include "ESPTelnetStream.h"
#include "driver/gpio.h"
#include <log.h>

// TODO aggiungere NTP per avere l'ora esatta (con connessione internet o con un server su jetson)

/*--------------ISTANZE--------------*/
ESPTelnetStream telnet;
uint16_t  port = 23;
Adafruit_ADS1015 ads;

// Mappatura porte seriali per 485 dedicati
HardwareSerial SerialROV(0);
HardwareSerial SerialLampSX(1);
HardwareSerial SerialLampDX(2);

RS485Bus ROV485(SerialROV, RW_485_ROV);
RS485Bus LampSX485(SerialLampSX, RW_485_LED_SX);
RS485Bus LampDX485(SerialLampDX, RW_485_LED_DX);

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

bool alreadyLogged[ERROR_LAST_INDEX] = {false}; // Array per tenere traccia degli errori già registrati

/*--------------FUNZIONI------------*/

// FUNZIONE PER LA DICHIARAZIONE DI PIN CONTENUTI IN UN ARRAY
void declaration_function(const uint8_t array[], uint8_t size,  byte type){
    uint8_t index_variable = 0;
    for (index_variable=0; index_variable< size; index_variable++){
        pinMode(array[index_variable], type);
    }
}

// FUNZIONE PER IMPOSTARE IL VALORE DEI PIN CONTENUTI IN UN ARRAY
void set_pin_function(const uint8_t array[], uint8_t size, byte value){
    uint8_t index_variable = 0;
    for (index_variable=0; index_variable< size; index_variable++){
        digitalWrite(array[index_variable], value);
    }
}

// FUNZIONE PER L'INIZIALIZZAZIONE DI TUTTE LE ISTANZE
void initialize(){
  // Inizializzazione PWM per PWM_LIGHT
  ledcSetup(0, 5000, 8);          // Canale 0, frequenza 5kHz, risoluzione 8 bit, per PWM
  ledcAttachPin(PWM_LIGHT, 0);    // Associa PWM_LIGHT al canale 0

  declaration_function(OUTPUT_ARRAY, sizeof(OUTPUT_ARRAY), OUTPUT);
  set_pin_function(OUTPUT_ARRAY, sizeof(OUTPUT_ARRAY), LOW);
  declaration_function(INPUT_ARRAY, sizeof(INPUT_ARRAY), INPUT);

  //Serial.begin(SERIAL_SPEED);
  SerialROV.begin(SERIAL_SPEED, SERIAL_8N1, RX_485_ROV, TX_485_ROV);           // begin RS485 ROV
  SerialLampSX.begin(SERIAL_SPEED, SERIAL_8N1, RX_485_LED_SX, TX_485_LED_SX);  // begin RS485 LED SX
  SerialLampDX.begin(SERIAL_SPEED, SERIAL_8N1, RX_485_LED_DX, TX_485_LED_DX);  // begin RS485 LED DX

  SerialROV.setTimeout(100);    // imposto il timeout del readStringUnitil per evitare che resti bloccato troppo a lungo
  SerialLampSX.setTimeout(100); // imposto il timeout del readStringUnitil per evitare che resti bloccato troppo a lungo
  SerialLampDX.setTimeout(100); // imposto il timeout del readStringUnitil per evitare che resti bloccato troppo a lungo

  Wire.begin(SDA_PIN, SCL_PIN);                   // begin i2c come master
  delay(10);

  //Serial.println(psramFound() ? "PSRAM Abilitata" : "PSRAM Disabilitata");

  EEPROM_Setup(); // Inizializza la EEPROM

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
  if(!logger.begin(true)) {                // Prova a montare il filesystem
    ROV485.sendMessageNoResponse("HUB Mount failed\n\r");    // Se fallisce, messaggio di errore
    return;
  }

  #ifdef LOG_DEBUG
    logger.checkFS(telnet);
  #endif

  logger.log("\n\r# " + String(systemStatus.power_cycle_count)); // Scrive nel log il numero di avvi della scheda

  tone(BUZZER_DEBUG, 300, 100);
}

// FUNZIONE PER IL SETUP DELLA EEPROM (LETTURA SCRITTURA VARIABILI DI SISTEMA INIZIALI)
void EEPROM_Setup(){
  EEPROM.begin(EEPROM_SIZE);                      // inizializzo la memoria da utilizzare
  /* Stampa ID */
  EEPROM.readBytes(ADDR_ID_NUM, systemStatus.ID, ID_NUM_SIZE);                          // Legge l'ID salvato nella EEPROM
  if (checkID(systemStatus.ID)) {                                                       // Verifico che l'ID sia valido
    writeTelnet("Device ID: " + String(systemStatus.ID));                               // Stampa l'ID del dispositivo
    ROV485.sendMessageNoResponse("HUB Device ID: " + String(systemStatus.ID) + "\n\r"); // Invia l'ID del dispositivo al ROV
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
}

void processWarning(uint8_t WarningCode){
  systemStatus.WarningFlags[WarningCode] = true;
  systemStatus.warning_detected = true;
}

void processError(uint8_t ErrorCode){
  systemStatus.ErrorFlags[ErrorCode] = true;
  systemStatus.error_detected = true;
  if (!alreadyLogged[ErrorCode]){
    logger.log(String(ErrorNames[ErrorCode]));
    logSystemData(); // Logga i dati di sistema al momento dell'errore
    alreadyLogged[ErrorCode] = true;
  }
}

void systemStatusCheck(){
  systemStatus.warning_detected = false;
  systemStatus.error_detected = false;

  // Check Overtemp
  if (systemStatus.ic12V_temperature > WARNING_OVERTEMP_12V_THRESHOLD){
    processWarning(WARNING_OVERTEMP_12V);
  } else systemStatus.WarningFlags[WARNING_OVERTEMP_12V] = false;
  
  if (systemStatus.ic12V_temperature > ERROR_OVERTEMP_12V_THRESHOLD){
    processError(ERROR_OVERTEMP_12V);
  } else systemStatus.ErrorFlags[ERROR_OVERTEMP_12V] = false;
  
  if (systemStatus.ic24V_temperature > WARNING_OVERTEMP_24V_THRESHOLD){
    processWarning(WARNING_OVERTEMP_24V);
  } else systemStatus.WarningFlags[WARNING_OVERTEMP_24V] = false;
  
  if (systemStatus.ic24V_temperature > ERROR_OVERTEMP_24V_THRESHOLD){
    processError(ERROR_OVERTEMP_24V);
  } else systemStatus.ErrorFlags[ERROR_OVERTEMP_24V] = false;


  // Check Undervoltage
  if (systemStatus.ic12V_voltage < WARNING_UNDERVOLTAGE_12V_THRESHOLD){
    processWarning(WARNING_UNDERVOLTAGE_12V);
  } else systemStatus.WarningFlags[WARNING_UNDERVOLTAGE_12V] = false;
  
  if (systemStatus.ic12V_voltage < ERROR_UNDERVOLTAGE_12V_THRESHOLD){
    processError(ERROR_UNDERVOLTAGE_12V);
  } else systemStatus.ErrorFlags[ERROR_UNDERVOLTAGE_12V] = false;
  
  if (systemStatus.ic24V_voltage < WARNING_UNDERVOLTAGE_24V_THRESHOLD){
    processWarning(WARNING_UNDERVOLTAGE_24V);
  } else systemStatus.WarningFlags[WARNING_UNDERVOLTAGE_24V] = false;
  
  if (systemStatus.ic24V_voltage < ERROR_UNDERVOLTAGE_24V_THRESHOLD){
    processError(ERROR_UNDERVOLTAGE_24V);
  } else systemStatus.ErrorFlags[ERROR_UNDERVOLTAGE_24V] = false;
  

  // Check Overvoltage
  if (systemStatus.ic12V_voltage > WARNING_OVERVOLTAGE_12V_THRESHOLD){
    processWarning(WARNING_OVERVOLTAGE_12V);
  } else systemStatus.WarningFlags[WARNING_OVERVOLTAGE_12V] = false;

  if (systemStatus.ic12V_voltage > ERROR_OVERVOLTAGE_12V_THRESHOLD){
    processError(ERROR_OVERVOLTAGE_12V);
  } else systemStatus.ErrorFlags[ERROR_OVERVOLTAGE_12V] = false;

  if (systemStatus.ic24V_voltage > WARNING_OVERVOLTAGE_24V_THRESHOLD){
    processWarning(WARNING_OVERVOLTAGE_24V);
  } else systemStatus.WarningFlags[WARNING_OVERVOLTAGE_24V] = false;

  if (systemStatus.ic24V_voltage > ERROR_OVERVOLTAGE_24V_THRESHOLD){
    processError(ERROR_OVERVOLTAGE_24V);
  } else systemStatus.ErrorFlags[ERROR_OVERVOLTAGE_24V] = false;


  // Check Overcurrent
  if (systemStatus.ipcam_current > WARNING_OVERCURRENT_IPCAM_THRESHOLD){
    processWarning(WARNING_OVERCURRENT_IPCAM);
  } else systemStatus.WarningFlags[WARNING_OVERCURRENT_IPCAM] = false;
  
  if (systemStatus.ipcam_current > ERROR_OVERCURRENT_IPCAM_THRESHOLD){
    processError(ERROR_OVERCURRENT_IPCAM);
  } else systemStatus.ErrorFlags[ERROR_OVERCURRENT_IPCAM] = false;
  
  if (systemStatus.bd3d_current > WARNING_OVERCURRENT_BD3D_THRESHOLD){
    processWarning(WARNING_OVERCURRENT_BD3D);
  } else systemStatus.WarningFlags[WARNING_OVERCURRENT_BD3D] = false;
  
  if (systemStatus.bd3d_current > ERROR_OVERCURRENT_BD3D_THRESHOLD){
    processError(ERROR_OVERCURRENT_BD3D);
  } else systemStatus.ErrorFlags[ERROR_OVERCURRENT_BD3D] = false;
  
  if (systemStatus.lamp1_current > WARNING_OVERCURRENT_LAMPSX_THRESHOLD){
    processWarning(WARNING_OVERCURRENT_LAMPSX);
  } else systemStatus.WarningFlags[WARNING_OVERCURRENT_LAMPSX] = false;
  
  if (systemStatus.lamp1_current > ERROR_OVERCURRENT_LAMPSX_THRESHOLD){
    processError(ERROR_OVERCURRENT_LAMPSX);
  } else systemStatus.ErrorFlags[ERROR_OVERCURRENT_LAMPSX] = false;
  
  if (systemStatus.lamp2_current > WARNING_OVERCURRENT_LAMPDX_THRESHOLD){
    processWarning(WARNING_OVERCURRENT_LAMPDX);
  } else systemStatus.WarningFlags[WARNING_OVERCURRENT_LAMPDX] = false;
  
  if (systemStatus.lamp2_current > ERROR_OVERCURRENT_LAMPDX_THRESHOLD){
    processError(ERROR_OVERCURRENT_LAMPDX);
  } else systemStatus.ErrorFlags[ERROR_OVERCURRENT_LAMPDX] = false;


  // Verifico che la lampada SX rispondano ogni "LAMP_COMMUNICATION_TIMEOUT" millisecondi
  if (millis() - systemStatus.last_lampSX_comm_time > LAMP_COMMUNICATION_TIMEOUT) {
    processWarning(WARNING_LAMPSX_DISCONNECTED);
  } else systemStatus.WarningFlags[WARNING_LAMPSX_DISCONNECTED] = false;


  // Verifico che la lampada DX rispondano ogni "LAMP_COMMUNICATION_TIMEOUT" millisecondi
  if (millis() - systemStatus.last_lampDX_comm_time > LAMP_COMMUNICATION_TIMEOUT) {
    processWarning(WARNING_LAMPDX_DISCONNECTED);
  } else systemStatus.WarningFlags[WARNING_LAMPDX_DISCONNECTED] = false;


  // Water leak detection
  if (digitalRead(WATER_PROBE_PIN) == HIGH){
    processError(ERROR_WATER_LEAK_DETECTED);
  } else systemStatus.ErrorFlags[ERROR_WATER_LEAK_DETECTED] = false;

  if (millis() - logger.lastlogtime > LOG_INTERVAL) {
    systemStatus.last_log_time = millis();
    logSystemData(); // Logga i dati di sistema in un formato CSV
  }

  // Stampa a sul 485 del ROV lo stato dei warning e degli errori ogni "PRINT_ERROR_TIMER" millisecondi
  if (millis() - systemStatus.last_error_print_time > PRINT_ERROR_TIMER){
    systemStatus.last_error_print_time = millis();

    if (systemStatus.warning_detected) {
      for (int i = 0; i < WARNING_LAST_INDEX; i++) {
        if (systemStatus.WarningFlags[i])
          ROV485.sendMessageNoResponse("\n\r" + String(WarningNames[i]) + ": " + (systemStatus.WarningFlags[i] ? "TRUE" : "FALSE"));
      }
    }

    if (systemStatus.error_detected) {
      for (int i = 0; i < ERROR_LAST_INDEX; i++) {
        if (systemStatus.ErrorFlags[i])
          ROV485.sendMessageNoResponse("\n\r" + String(ErrorNames[i]) + ": " + (systemStatus.ErrorFlags[i] ? "TRUE" : "FALSE"));
      }
    }

    #ifdef DEBUG_STATUS
      printSystemStatus();
    #endif
  }
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
    writeTelnet("Lamp SX Ready:" + String(systemStatus.lampSX_Ready ? "TRUE" : "FALSE"));
    writeTelnet("Lamp DX Ready:" + String(systemStatus.lampDX_Ready ? "TRUE" : "FALSE"));

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

void logSystemData(){
    String out;
    // Riga di intestazione per il file CSV
    //String header = "Millis,ID,PowerCycle,LampSX,LampDX,Lamp1C,Lamp2C,24VV,24VT,24C0,24C1,BD3D,IPCam,12VV,12VT,12C0,12C1,LRF,LSReady,LDReady";

    out  = String(millis());
    out += "," + String(systemStatus.ID);
    out += "," + String(systemStatus.power_cycle_count);
    out += "," + String(systemStatus.last_lampSX_comm_time);
    out += "," + String(systemStatus.last_lampDX_comm_time);
    out += "," + String(systemStatus.lamp1_current, 2);
    out += "," + String(systemStatus.lamp2_current, 2);
    out += "," + String(systemStatus.ic24V_voltage, 2);
    out += "," + String(systemStatus.ic24V_temperature, 1);
    out += "," + String(systemStatus.ic24V_C0_state);
    out += "," + String(systemStatus.ic24V_C1_state);
    out += "," + String(systemStatus.bd3d_current, 2);
    out += "," + String(systemStatus.ipcam_current, 2);
    out += "," + String(systemStatus.ic12V_voltage, 2);
    out += "," + String(systemStatus.ic12V_temperature, 1);
    out += "," + String(systemStatus.ic12V_C0_state);
    out += "," + String(systemStatus.ic12V_C1_state);
    out += "," + String(systemStatus.lamp_reset_flag);
    out += "," + String(systemStatus.lampSX_Ready);
    out += "," + String(systemStatus.lampDX_Ready);

    for (int i = 0; i < WARNING_LAST_INDEX; i++) {
        out += "," + String(systemStatus.WarningFlags[i]);
    }
    for (int i = 0; i < ERROR_LAST_INDEX; i++) {
        out += "," + String(systemStatus.ErrorFlags[i]);
    }
    out += "\n\r";

    logger.log(out); // Scrive la riga di dati nel file di log
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

// FUNZIONE CHE CHIEDE L'ID ALLE LAMPADE CONNESSE E LO METTE IN MEMORIA
bool readLampID(){
  bool sx_flag = false, dx_flag = false;

  String responseSX = LampSX485.sendMessage("id_print\r\n");                // Mi faccio inviare l'ID dalle lampade
  String responseDX = LampDX485.sendMessage("id_print\r\n");                // Mi faccio inviare l'ID dalle lampade

  writeTelnet("LAMPSX_ID: " + responseSX);
  writeTelnet("LAMPDX_ID: " + responseDX);

  if (responseSX.startsWith("FLS")){
    strcpy(systemStatus.ID_lampSX, responseSX.c_str());
    sx_flag = true;
  }

  if (responseDX.startsWith("FLS")){
    strcpy(systemStatus.ID_lampDX, responseDX.c_str());
    dx_flag = true;
  }

  return sx_flag || dx_flag;
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

void FlashReadyCheck(){
  bool is_lampsx_torch = false, is_lampdx_torch = false;

  String responseSX = LampSX485.sendMessage("status\r\n");                        // Invio del comando di status al flash DX tramite 485
  if (responseSX == "1111"){            // Se la risposta è "1111" significa che la lampada è pronta allo scatto
    systemStatus.last_lampSX_comm_time = millis();
    systemStatus.lampSX_Ready = true;
  }
  else if (responseSX == "0101"){       // Se la risposta è "0101" significa che la lampada è in modalità torcia
    systemStatus.last_lampSX_comm_time = millis();
    systemStatus.lampSX_Ready = false;
    is_lampsx_torch = true;
  }
  else if (responseSX == "0000"){
    systemStatus.last_lampSX_comm_time = millis();
    systemStatus.lampSX_Ready = false;
  }
  else systemStatus.lampSX_Ready = false;

  String responseDX = LampDX485.sendMessage("status\r\n");                        // Invio del comando di status al flash SX tramite 485
  if (responseDX == "1111"){            // Se la risposta è "1111" significa che la lampada è pronta allo scatto
    systemStatus.last_lampDX_comm_time = millis();
    systemStatus.lampDX_Ready = true;
  }
  else if (responseDX == "0101"){       // Se la risposta è "0101" significa che la lampada è in modalità torcia
    systemStatus.last_lampDX_comm_time = millis();
    systemStatus.lampDX_Ready = false;
    is_lampdx_torch = true;
  }
  else if (responseDX == "0000"){
    systemStatus.last_lampDX_comm_time = millis();
    systemStatus.lampDX_Ready = false;
  }
  else systemStatus.lampDX_Ready = false;

  systemStatus.is_torch_mode = is_lampsx_torch || is_lampdx_torch;

}

// FUNZIONE CHE INOLTRA IL COMANDO DI RESET ALLE LAMPADE TRAMITE 485
void resetLamp(){
  LampSX485.sendMessageNoResponse("reset\n\r");                     // Invio del comando di reset ai flash tramite 485
  LampDX485.sendMessageNoResponse("reset\n\r");                     // Invio del comando di reset ai flash tramite 485
  ROV485.sendMessageNoResponse("Lamp reset sent\n\r");              // Invio del comando di reset effettuato al ROV tramite 485
  writeTelnet("Lamp reset sent");                                   // Stampa il comando di reset effettuato sulla telnet
}

void blinkDebugLED(uint8_t pin){
  digitalWrite(pin, HIGH);
  delay(BLINK_DELAY_TIME);
  digitalWrite(pin, LOW);
}

String getHTMLpage(){
  String html_String = "";
  File file = LittleFS.open("/ui.html", "r");
  if(!file){
    writeTelnet("Failed to open ui.html");
  }
  while(file.available()){
      html_String += char(file.read());
  }
  file.close();
  return html_String;
}

// FUNZIONE CHE VERIFICA SE L'ID PASSATO SIA VALIDO
bool checkID(char ID_to_check[ID_NUM_SIZE]){
  return (ID_to_check[0] == 'H' & ID_to_check[1] == 'U' & ID_to_check[2] == 'B');
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
  telnet.println("- Telnet: " + ip + " disconnected");
}

/*
void onTelnetReconnect(String ip) {
  //Serial.print("- Telnet: ");
  //Serial.print(ip);
  //Serial.println(" reconnected");
}

void onTelnetConnectionAttempt(String ip) {
  //Serial.print("- Telnet: ");
  //Serial.print(ip);
  //Serial.println(" tried to connected");
}

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
  */

void setupTelnet() {  
  // passing on functions for various telnet events
  telnet.onConnect(onTelnetConnect);
  telnet.onDisconnect(onTelnetDisconnect);
  //telnet.onConnectionAttempt(onTelnetConnectionAttempt);
  //telnet.onReconnect(onTelnetReconnect);
  //telnet.onInputReceived(onTelnetInput);

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
  telnet.println(text);
}

// FUNZIONE CHE STAMPA SU TELNET I BUS 485 SE TROVA DEI DATI DA LEGGERE
void print485onTelnet(){
  String response = "";

  response = ROV485.checkMessage(MIN_RX_BYTE_485);
  if (response != "")
    writeTelnet(response);

  response = LampSX485.checkMessage(MIN_RX_BYTE_485);
  if (response != "")
    writeTelnet(response);

  response = LampDX485.checkMessage(MIN_RX_BYTE_485);
  if (response != "")
    writeTelnet(response);
}