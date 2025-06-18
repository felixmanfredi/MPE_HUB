#include <HUB_firmware.h>
#include "ESPTelnetStream.h"

/*--------------ISTANZE--------------*/
ESPTelnetStream telnet;
uint16_t  port = 23;

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

// FUNZIONE PER L'INIZIAIZZAZIONE DI TUTTE LE ISTANZE
void initialize(){
    declaration_function(OUTPUT_ARRAY, sizeof(OUTPUT_ARRAY), OUTPUT);
    set_pin_function(OUTPUT_ARRAY, sizeof(OUTPUT_ARRAY), LOW);              // Apro tutti gli interruttori
    declaration_function(INPUT_ARRAY, sizeof(INPUT_ARRAY), INPUT);

    Serial.begin(SERIAL_SPEED);                                                     // begin porta seriale USB
    Serial2.begin(SERIAL_SPEED, SERIAL_8N1, RX_485, TX_485);                        // begin RS485

    Serial.println(psramFound() ? "PSRAM Abilitata" : "PSRAM Disabilitata");

    pinMode(RST_GPIO, OUTPUT);
    digitalWrite(RST_GPIO, HIGH);
    
    tone(BUZZER_DEBUG, 300, 100);
}

// FUNZIONE CHE STAMPA IL TESTO SUL 485
void write485(String text){
    digitalWrite(RW_485,HIGH);                      // attivazione scrittura 485
    delay(2);
    Serial2.print(text);                            // stampa il testo in 485
    Serial.print(text);                             // stampa il testo sulla seriale
    delay(2);
    digitalWrite(RW_485,LOW);                       // attivazione lettura 485
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
    Serial.print(".");
    i++;
  } while (!isConnected() && i < max_tries);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  return isConnected();
}

void errorMsg(String error, bool restart = true) {
  Serial.println(error);
  if (restart) {
    Serial.println("Rebooting now...");
    delay(2000);
    ESP.restart();
    delay(2000);
  }
}

// (optional) callback functions for telnet events
void onTelnetConnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" connected");
  
  telnet.println("\nWelcome " + telnet.getIP());
  telnet.println("(Use ^] + q  to disconnect.)");
}

/* ------------------------------------------------- */

void onTelnetDisconnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" disconnected");
}

/* ------------------------------------------------- */

void onTelnetReconnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" reconnected");
}

/* ------------------------------------------------- */

void onTelnetConnectionAttempt(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" tried to connected");
}

/* ------------------------------------------------- */

void onTelnetInput(String str) {
  // checks for a certain command
  if (str == "ping") {
    telnet.println("> pong"); 
    Serial.println("- Telnet: pong");
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

  Serial.print("- Telnet: ");
  if (telnet.begin(port)) {
    Serial.println("running");
  } else {
    Serial.println("error.");
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
    digitalWrite(LED_DEBUG_GREEN, HIGH);
    delay(20);
    telnet.println(text);
    digitalWrite(LED_DEBUG_GREEN, LOW);
}