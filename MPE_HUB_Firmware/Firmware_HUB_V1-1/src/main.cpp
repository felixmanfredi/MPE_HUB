#include <VND70.h>
#include <HUB_firmware.h>
#include <AsyncWebServer_ESP32_SC_W5500.h>
#include <ElegantOTA.h>
#include <SimpleFTPServer.h>

// TODO resettare tutta la flash prima di usare coem definitivo

/*------------COMANDI CLI------------*/
SimpleCLI cli;                                  // Oggetto per CLI
Command ping;
Command set;
Command standby;
Command ID_set;
Command ID_print;
Command help;

/*------------VARIABILI-------------*/

const char* ssid = "HUB_MPE";
const char* password = "00000000";

IPAddress IP;
AsyncWebServer server(80);

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01 };

// Select the IP address according to your local network
IPAddress myIP(192, 168, 1, 231);
IPAddress myGW(192, 168, 1, 1);
IPAddress mySN(255, 255, 255, 0);
// Google DNS Server IP
IPAddress myDNS(8, 8, 8, 8);

// FTP server instance
FtpServer ftpSrv;

String jsonString;
DynamicJsonDocument jsonDoc(4096);                  // Creazione del json

struct flash_wrapper {
    String power_value = "0";
    bool flash_action_flag = false;
    String torch_value = "0";
    bool torch_action_flag = false;
} flash_wrapper;

struct id_num_wrapper {
    char value[ID_NUM_SIZE] = {0};  // Inizializzazione dell'ID
    bool action_flag = false;       // Flag per indicare se bisogna eseguire la funzione associata al comando
} id_num_wrapper;

/*------------FUNZIONI---------------*/

void jsonSerialize(){

    jsonDoc.clear();

    systemStatus.lamp1_current      = VND70::readCurrent(2, FLASH_1_CHANNEL);
    systemStatus.lamp2_current      = VND70::readCurrent(2, FLASH_2_CHANNEL);
    systemStatus.ic24V_voltage      = VND70::readVoltage(2);
    systemStatus.ic24V_temperature  = VND70::readTemperature(2);
    systemStatus.ic24V_C0_state     = VND70::channel_0_state(2);
    systemStatus.ic24V_C1_state     = VND70::channel_1_state(2);

    systemStatus.bd3d_current       = VND70::readCurrent(1, BD3D_CHANNEL);
    systemStatus.ipcam_current      = VND70::readCurrent(1, IPCAM_CHANNEL);
    systemStatus.ic12V_voltage      = VND70::readVoltage(1);
    systemStatus.ic12V_temperature  = VND70::readTemperature(1);
    systemStatus.ic12V_C0_state     = VND70::channel_0_state(1);
    systemStatus.ic12V_C1_state     = VND70::channel_1_state(1);

    jsonDoc.clear();

    jsonDoc["Board_ID"]             = systemStatus.ID;
    jsonDoc["Board_REV"]            = systemStatus.Board_REV;
    jsonDoc["FW_VERS"]              = systemStatus.FW_VERS;
    jsonDoc["Current_Flash_1"]      = systemStatus.lamp1_current;
    jsonDoc["Current_Flash_2"]      = systemStatus.lamp2_current;
    jsonDoc["Voltage_24V"]          = systemStatus.ic24V_voltage;
    jsonDoc["Chip_Temp_24V"]        = systemStatus.ic24V_temperature;
    jsonDoc["24V_0_State"]          = systemStatus.ic24V_C0_state;
    jsonDoc["24V_1_State"]          = systemStatus.ic24V_C1_state;
    jsonDoc["Current_BD3D"]         = systemStatus.bd3d_current;
    jsonDoc["Current_IPCam"]        = systemStatus.ipcam_current;
    jsonDoc["Voltage_12V"]          = systemStatus.ic12V_voltage;
    jsonDoc["Chip_Temp_12V"]        = systemStatus.ic12V_temperature;
    jsonDoc["12V_0_State"]          = systemStatus.ic12V_C0_state;
    jsonDoc["12V_1_State"]          = systemStatus.ic12V_C1_state;
    jsonDoc["LampSX_ID"]            = String(systemStatus.ID_lampSX);
    jsonDoc["LampDX_ID"]            = String(systemStatus.ID_lampDX);
    jsonDoc["LampSX_Disconnected"]  = systemStatus.WarningFlags[WARNING_LAMPSX_DISCONNECTED];
    jsonDoc["LampDX_Disconnected"]  = systemStatus.WarningFlags[WARNING_LAMPDX_DISCONNECTED];
    jsonDoc["LampSX_Ready"]         = systemStatus.lampSX_Ready;
    jsonDoc["LampDX_Ready"]         = systemStatus.lampDX_Ready;
    serializeJson(jsonDoc, jsonString);
}

/* COMANDI PER ESECUZIONE SINCRONA */

/*
* Funzione che imposta lo stato dei componenti
*/
void setCommand(String component, String action){
    if (component == "ipcam"){
        blinkDebugLED(LED_DEBUG_GREEN);
        if (action == "on"){
            VND70::channel_0(1, true);
        } else if (action == "off"){
            VND70::channel_0(1, false);
        }
        
    } else if (component == "bd3d"){
        blinkDebugLED(LED_DEBUG_GREEN);
        if (action == "on"){
            VND70::channel_1(1, true);
        } else if (action == "off"){
            VND70::channel_1(1, false);
        }

    } else if (component == "lampsx"){
        blinkDebugLED(LED_DEBUG_GREEN);
        if (action == "on"){
            VND70::channel_0(2, true);
            LampSX485.enableBus();
        } else if (action == "off"){        // Spengo la lampada e disattivo il bus 485
            VND70::channel_0(2, false);
            LampSX485.disableBus();
        }

    } else if (component == "lampdx"){
        blinkDebugLED(LED_DEBUG_GREEN);
        if (action == "on"){
            VND70::channel_1(2, true);
            LampDX485.enableBus();
        } else if (action == "off"){        // Spengo la lampada e disattivo il bus 485
            VND70::channel_1(2, false);
            LampDX485.disableBus();
        }
    } else if (component == "lamp"){
        blinkDebugLED(LED_DEBUG_GREEN);
        if (action == "reset") {
            resetLamp();
        } else if (action == "standby") {
            LampSX485.sendMessageNoResponse("\nflash " + action + "\n\r");            // Invio del comando di standby ai flash tramite 485
            LampDX485.sendMessageNoResponse("\nflash " + action + "\n\r");            // Invio del comando di standby ai flash tramite 485
        } else if (action == "idle") {
            LampSX485.sendMessageNoResponse("\nflash " + action + "\n\r");            // Invio del comando di idle ai flash tramite 485
            LampDX485.sendMessageNoResponse("\nflash " + action + "\n\r");            // Invio del comando di idle ai flash tramite 485
        } else if (action == "flash") {
            LampSX485.sendMessageNoResponse("\nflash " + action + "\n\r");            // Invio del comando di flash ai flash tramite 485
            LampDX485.sendMessageNoResponse("\nflash " + action + "\n\r");            // Invio del comando di flash ai flash tramite 485
        }

    } else if (component == "lamp_power") {
        blinkDebugLED(LED_DEBUG_GREEN);
        int power = action.toInt();
        if (power >= 0 && power <= MAX_POWER_FLASH) {
            LampSX485.sendMessageNoResponse("power_flash " + action  + "\n\r");
            LampDX485.sendMessageNoResponse("power_flash " + action  + "\n\r");
        }

    } else if (component == "lamp_torch") {
        blinkDebugLED(LED_DEBUG_GREEN);
        int power = action.toInt();
        if (power == 0){
            LampSX485.sendMessageNoResponse("torch 0\n\r");
            LampDX485.sendMessageNoResponse("torch 0\n\r");
        } else if (power == 1) {
            LampSX485.sendMessageNoResponse("torch 1\n\r");
            LampDX485.sendMessageNoResponse("torch 1\n\r");
        }

    }  else if (component == "light"){
        blinkDebugLED(LED_DEBUG_GREEN);
        int pwm_light = action.toInt();
        if (pwm_light >= 0 && pwm_light <= 255){        
            if (pwm_light != 0){
                VND70::channel_0(1, true);          // Accendo il canale IPcam e Lights
                ledcWrite(0, pwm_light);
            } else {
                ledcWrite(0, 0);                    // spegni PWM
            }
        } else {
            ROV485.sendMessageNoResponse("PWM light out of pwm range\n\r");
        }
    }
}

void ID_setCommand(char id_num[ID_NUM_SIZE]){
    if (id_num[0] == 'H' && id_num[1] == 'B' && id_num[2] == 'S' && id_num[3] == 'W'){    // Verifica che si riferisca all'HUB
        writeTelnet("# Comando ricevuto: ID_SET " + String(id_num));        // Stampa l'ID ricevuto
        id_num [ID_NUM_SIZE - 1] = '\0';                                    // Aggiungo il terminatore di stringa
        EEPROM.writeBytes(ADDR_ID_NUM, id_num, ID_NUM_SIZE);                // Salva l'ID nella EEPROM
        EEPROM.commit();
        memset(id_num, 0, ID_NUM_SIZE);                                     // resetto la stringa
        EEPROM.readBytes(ADDR_ID_NUM, id_num, ID_NUM_SIZE);                 // Legge l'ID salvato nella EEPROM
        writeTelnet("# ID_NUM salvato: " + String(id_num));
    }
    else {
        writeTelnet("ID non valido");
    }
}

void ID_printCommand(){
    char id_num[ID_NUM_SIZE] = {0};
    EEPROM.readBytes(ADDR_ID_NUM, id_num, ID_NUM_SIZE);     // Legge l'ID salvato nella EEPROM
    id_num[ID_NUM_SIZE - 1] = '\0';                         // Aggiungo il terminatore di stringa
    writeTelnet("ID del dispositivo: " + String(id_num));   // Stampa l'ID del dispositivo
}

/*-------------------------------------- OTA UPDATE --------------------------------------*/

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void onOTAStart() {
    // Log when OTA has started
    //SerialROV.println("OTA update started!");
    // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
    // Log every 1 second
    if (millis() - systemStatus.ota_progress_millis > 1000) {
        systemStatus.ota_progress_millis = millis();
        //SerialROV.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
    }
}

void onOTAEnd(bool success) {
    // Log when OTA has finished
    if (success) {
        //SerialROV.println("OTA update finished successfully!");
    } else {
        //SerialROV.println("There was an error during OTA update!");
    }
    // <Add your own code here>
}

/*-------------------------------------- SERVER INITIALIZE -----------------------------------------*/
// TODO aggiungere stato flash e seriali
void server_initialize(){
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", getHTMLpage());});      // HTML in littlefs
        //request->send(200, "text/html", MAIN_page);});        // HTML in .h
        //request->send(200, "text/plain", "HUB MPE");});       // simple page
    
    server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", jsonString);             // Invio della risposta
    });

    server.on("/lamp/torch", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("power")) {
            String Power = request->getParam("power")->value();
            request->send(200, "text/plain", "Torch power set to: " + Power);
            flash_wrapper.power_value = "0";
            flash_wrapper.flash_action_flag = false;
            flash_wrapper.torch_value = Power;
            flash_wrapper.torch_action_flag = true;
        } else {
            request->send(400, "text/plain", "Missing 'power' parameter");
        }
    });

    server.on("/lamp/power", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("power")) {
            String Power = request->getParam("power")->value();
            request->send(200, "text/plain", "Lamp power set to: " + Power);
            flash_wrapper.power_value = Power;
            flash_wrapper.flash_action_flag = true;
            flash_wrapper.torch_value = "0";
            flash_wrapper.torch_action_flag = false;
        } else {
            request->send(400, "text/plain", "Missing 'power' parameter");
        }
    });

    server.on("/lamp/reset", HTTP_GET, [](AsyncWebServerRequest *request){

        systemStatus.lamp_reset_flag = true; // Imposto il flag per eseguire il reset delle lampade nel loop principale

        if(SerialROV.available()) {
            String response = SerialROV.readStringUntil('\n');                    // Leggo la risposta dal seriale
            request->send(200, "text/plain", "Lamp reset error sent.\n Lamp response: " + response);
        } else {
            request->send(200, "text/plain", "Lamp reset error sent.\n No response.");
        }
    });

    server.on("/lampSX", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("state")) {
            String Status = request->getParam("state")->value();
            cli.parse("set LampSX " + Status);
            request->send(200, "text/plain", "LampSX set to: " + Status);
        } else {
            request->send(400, "text/plain", "Missing 'state' parameter");
        }
    });

    server.on("/lampDX", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("state")) {
            String Status = request->getParam("state")->value();
            cli.parse("set LampDX " + Status);
            request->send(200, "text/plain", "LampDX set to: " + Status);
        } else {
            request->send(400, "text/plain", "Missing 'state' parameter");
        }
    });

    server.on("/IPCam", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("state")) {
            String Status = request->getParam("state")->value();
            cli.parse("set IPCam " + Status);
            request->send(200, "text/plain", "IPCam set to: " + Status);
        } else {
            request->send(400, "text/plain", "Missing 'state' parameter");
        }
    });

    server.on("/BD3D", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("state")) {
            String Status = request->getParam("state")->value();
            cli.parse("set BD3D " + Status);
            request->send(200, "text/plain", "BD3D set to: " + Status);
        } else {
            request->send(400, "text/plain", "Missing 'state' parameter");
        }
    });

    server.onNotFound(notFound);
}

/* ------------------------------------------- FTP ------------------------------------------- */

void _callback(FtpOperation ftpOperation, unsigned int freeSpace, unsigned int totalSpace){
  switch (ftpOperation) {
    case FTP_CONNECT:
      Serial.println(F("FTP: Connected!"));
      break;
    case FTP_DISCONNECT:
      Serial.println(F("FTP: Disconnected!"));
      break;
    case FTP_FREE_SPACE_CHANGE:
      Serial.printf("FTP: Free space change, free %u of %u!\n", freeSpace, totalSpace);
      break;
    default:
      break;
  }
};

void _transferCallback(FtpTransferOperation ftpOperation, const char* name, unsigned int transferredSize){
  switch (ftpOperation) {
    case FTP_UPLOAD_START:
      Serial.println(F("FTP: Upload start!"));
      break;
    case FTP_UPLOAD:
      Serial.printf("FTP: Upload of file %s byte %u\n", name, transferredSize);
      break;
    case FTP_TRANSFER_STOP:
      Serial.println(F("FTP: Finish transfer!"));
      break;
    case FTP_TRANSFER_ERROR:
      Serial.println(F("FTP: Transfer error!"));
      break;
    default:
      break;
  }

  /* FTP_UPLOAD_START = 0,
   * FTP_UPLOAD = 1,
   *
   * FTP_DOWNLOAD_START = 2,
   * FTP_DOWNLOAD = 3,
   *
   * FTP_TRANSFER_STOP = 4,
   * FTP_DOWNLOAD_STOP = 4,
   * FTP_UPLOAD_STOP = 4,
   *
   * FTP_TRANSFER_ERROR = 5,
   * FTP_DOWNLOAD_ERROR = 5,
   * FTP_UPLOAD_ERROR = 5
   */
};

/* ------------------------------------------- CLI ------------------------------------------- */

// Callback per il comando set
void setCallback(cmd* c) {
    Command cmd(c); // Create wrapper object
    Argument compArg = cmd.getArgument("component");
    String compValue = compArg.getValue();
    compValue.toLowerCase();
    Argument actionArg = cmd.getArgument("action");
    String actionValue = actionArg.getValue();
    actionValue.toLowerCase();
    setCommand(compValue, actionValue); // Esegue il comando set
}

// Callback per la modalità standby
void standbyCallback(cmd* c) {
    Command cmd(c); // Create wrapper object
    //write485("Comando ricevuto: Stand-by");
    VND70::standby(1);
    VND70::standby(2);
}

// Callback per il comando help
void ID_setCallback(cmd* c){
    Command cmd(c); //create wrapper
    Argument stateArg = cmd.getArgument("id_num");
    strncpy(id_num_wrapper.value, stateArg.getValue().c_str(), ID_NUM_SIZE); // Copia il valore dell'ID dal comando
    id_num_wrapper.action_flag = true;                     // Imposto il flag per eseguire la funzione associata alla scrittura dell'ID
}

// Callback per il comando ID_print
void ID_printCallback(cmd* c){
    systemStatus.id_print_flag = true; // Imposto il flag per eseguire la funzione associata alla stampa dell'ID
}

void helpCallback(cmd* c){
    writeTelnet("Help: " + cli.toString() + "\n");
    /*write485("Help: ");
    write485(cli.toString() + "\n");*/
}

// Callback per gli errori della CLI
void errorCallback(cmd_error* e) {
    CommandError cmdError(e); // Create wrapper object

    //SerialROV.print("ERROR: ");
    //SerialROV.println(cmdError.toString());

    if (cmdError.hasCommand()) {
        //SerialROV.print("Did you mean \"");
        //SerialROV.print(cmdError.getCommand().toString());
        //SerialROV.println("\"?");
    }
}


/*--------------SETUP--------------*/

void setup() {
    initialize();

    // Istanze dei due VND70 
    VND70::registerComponent(1, MULTISENSE_12V_ADC_PIN, ENABLE_0_12V, ENABLE_1_12V, SEL_0_12V, SEL_1_12V);  // ID=1
    VND70::registerComponent(2, MULTISENSE_24V_ADC_PIN, ENABLE_0_24V, ENABLE_1_24V, SEL_0_24V, SEL_1_24V);  // ID=2
    VND70::begin();

    ESP32_W5500_onEvent();
    delay(1000);
    ETH.begin( MISO_GPIO, MOSI_GPIO, SCK_GPIO, CS_GPIO, INT_GPIO, SPI_CLOCK_MHZ, ETH_SPI_HOST, mac);
    ETH.config(myIP, myGW, mySN, myDNS);
    //ESP32_W5500_waitForConnect();

    /*--WIFI--*/
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    IP = WiFi.softAPIP();
    //SerialROV.print("AP IP address: ");
    //SerialROV.println(IP);
    ftpSrv.setCallback(_callback);
    ftpSrv.setTransferCallback(_transferCallback);
    ftpSrv.begin("HUB_MPE","1999");    //username, password for ftp.   (default 21, 50009 for PASV)

    server_initialize();

    ElegantOTA.begin(&server);                              // Start ElegantOTA
    ElegantOTA.onStart(onOTAStart);
    ElegantOTA.onProgress(onOTAProgress);
    ElegantOTA.onEnd(onOTAEnd);
    server.begin();
    //SerialROV.print("HTTP server started with IP:");
    //SerialROV.println(ETH.localIP());

    setupTelnet();                                 // Inizializzo telnet

    /* Setup e verifica comandi CLI */
    cli.setCaseSensetive(false);

    set = cli.addCmd("set", setCallback);
    set.addPositionalArgument("component");
    set.addPositionalArgument("action");
    set.setDescription( "Esegui una determianta \'action\' (on - off) su uno specifico \'component\' (IPcam - BD3D - Lamp - Lamp_power - Lamp_torch)\n\r" 
                        "Esempio: \n\r# set Lamp on\n\n\r"
                        "Esempio impostazione modalità illuminatore continuo: \n\r#set Lamp_torch (0 to 3)\n\n\r"
                        "Esempio impostazione luci ausiliarie: \n\r#set light (0 to 255)\n\n\r"
                        "Esempio impostazione potenza degli illuminatori: \n\r#set Lamp_power (0 to 4)");

    standby = cli.addCmd("standby", standbyCallback);
    standby.setDescription("Porta allo stato standby tutto il sistema");

    ID_set = cli.addCommand("ID_set", ID_setCallback);
    ID_set.addPositionalArgument("id_num", "00000000");

    ID_print = cli.addCommand("ID_print", ID_printCallback);
    ID_print.setDescription("Stampa l'ID del dispositivo");

    help = cli.addCommand("help", helpCallback);
    help.setDescription("Panoramica dei comandi");

    cli.setOnError(errorCallback); // Set error Callback

    digitalWrite(LED_DEBUG_RED, HIGH);      // RED
    digitalWrite(LED_DEBUG_GREEN, HIGH);    // GREEN
    tone(BUZZER_DEBUG, 600, 50);
    tone(BUZZER_DEBUG, 300, 100);
    delay(1000);
    VND70::channel_0(1, true);              // Temporizzo le accensioni per evitare assorbimenti elevati
    delay(1000);
    VND70::channel_1(1, true);
    delay(1000);
    VND70::channel_0(2, true);
    delay(1000);
    VND70::channel_1(2, true);
    tone(BUZZER_DEBUG, 600, 100);
    tone(BUZZER_DEBUG, 1200, 200);

    delay(5000);
    set_pin_function(OUTPUT_ARRAY, sizeof(OUTPUT_ARRAY), LOW);  // Porto a LOW tutte le uscite
    tone(BUZZER_DEBUG, 1200, 100);
    tone(BUZZER_DEBUG, 1800, 200);
}

bool id_not_set = false;

void loop() {
    #ifdef DEBUG
        scanI2C();
        print_ADC();
    #endif

    delay(5);
    ElegantOTA.loop();
    ftpSrv.handleFTP();
    String input = loopTelnet();

    jsonSerialize();

    systemStatusCheck();

    // TODO aggiungere funzione per avviare una procedura di spegnimento in caso di errori
    if (systemStatus.error_detected){
        digitalWrite(LED_DEBUG_RED, HIGH);     // Accendo il LED rosso
    } else {
        digitalWrite(LED_DEBUG_RED, LOW);      // Spengo il LED rosso
    }

    if (flash_wrapper.flash_action_flag) {
        flash_wrapper.flash_action_flag = false;
        setCommand("lamp_power", flash_wrapper.power_value); // Imposta lo stato dell'illuminatore
    }

    if (flash_wrapper.torch_action_flag) {
        flash_wrapper.torch_action_flag = false;
        setCommand("lamp_torch", flash_wrapper.torch_value); // Imposta lo stato dell'illuminatore
    }

    if(id_num_wrapper.action_flag) {
        id_num_wrapper.action_flag = false;
        ID_setCommand(id_num_wrapper.value);   // Imposta l'ID del dispositivo
    }

    if(systemStatus.id_print_flag) {
        systemStatus.id_print_flag = false;
        ID_printCommand();                     // Stampa l'ID del dispositivo
    }

    if(systemStatus.lamp_reset_flag) {
        systemStatus.lamp_reset_flag = false;
        resetLamp();                          // Esegue il reset delle lampade
    }

    if (input != "") {
        #ifdef ECO485
            write485ROV("# " + input + "\n\r");                  // genera eco su seriale e 485
        #endif
        cli.parse(input);                                    // manda l'input alla CLI
    }

    print485onTelnet();             // Rimando su telnet i messaggi che arrivano dai 485

    if (millis() - systemStatus.last_lamp_check_time > LAMP_CHECK_STATUS_TIMER){
        systemStatus.last_lamp_check_time = millis();
        FlashReadyCheck(); // Controlla se le lampade sono pronte allo scatto

        if (!systemStatus.isLampID_set)
            systemStatus.isLampID_set = readLampID();
    }

}
