#include <VND70.h>
#include <HUB_firmware.h>
#include <AsyncWebServer_ESP32_SC_W5500.h>
#include <ElegantOTA.h>

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

struct systemStatus {
    unsigned long ota_progress_millis = 0;
    unsigned long last_lampSX_comm_time = 0;
    unsigned long last_lampDX_comm_time = 0;
    bool lampSX_isAlive = false;
    bool lampDX_isAlive = false;
    float lamp1_current;
    float lamp2_current;
    float ic24V_voltage;
    float ic24V_temperature;
    bool  ic24V_C0_state;
    bool  ic24V_C1_state;
    float bd3d_current;
    float ipcam_current;
    float ic12V_voltage;
    float ic12V_temperature;
    bool  ic12V_C0_state;
    bool  ic12V_C1_state;
    uint8_t last_error = 0;
    bool id_print_flag = false;             // flag per la stampa dell'ID
    bool lamp_reset_flag = false;           // flag per il reset delle lampade
} systemStatus;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01 };

// Select the IP address according to your local network
IPAddress myIP(192, 168, 1, 231);
IPAddress myGW(192, 168, 1, 1);
IPAddress mySN(255, 255, 255, 0);
// Google DNS Server IP
IPAddress myDNS(8, 8, 8, 8);

String jsonString;
DynamicJsonDocument jsonDoc(4096);                  // Creazione del json

struct flash_wrapper {
    String power_value = "0";
    bool flash_action_flag = false;
    String torch_value = "0";
    bool torch_action_flag = false;
}flash_wrapper;

struct id_num_wrapper {
    char value[ID_NUM_SIZE] = {0};  // Inizializzazione dell'ID
    bool action_flag = false;       // Flag per indicare se bisogna eseguire la funzione associata al comando
} id_num_wrapper;

/*------------FUNZIONI---------------*/

void jsonSerialize(){
    
    systemStatus.lamp1_current = VND70::readCurrent(2, FLASH_1_CHANNEL);
    systemStatus.lamp2_current = VND70::readCurrent(2, FLASH_2_CHANNEL);
    systemStatus.ic24V_voltage = VND70::readVoltage(2);
    systemStatus.ic24V_temperature = VND70::readTemperature(2);
    systemStatus.ic24V_temperature = getAnalogueVoltage(MULTISENSE_24V_ADC_PIN);
    systemStatus.ic24V_C0_state = VND70::channel_0_state(2);
    systemStatus.ic24V_C1_state = VND70::channel_1_state(2);

    systemStatus.bd3d_current = VND70::readCurrent(1, BD3D_CHANNEL);
    systemStatus.ipcam_current = VND70::readCurrent(1, IPCAM_CHANNEL);
    systemStatus.ic12V_voltage = VND70::readVoltage(1);
    systemStatus.ic12V_temperature = VND70::readTemperature(1);
    systemStatus.ic12V_C0_state = VND70::channel_0_state(1);
    systemStatus.ic12V_C1_state = VND70::channel_1_state(1);

    jsonDoc.clear();
    jsonDoc["Current_Flash_1"] = systemStatus.lamp1_current;
    jsonDoc["Current_Flash_2"] = systemStatus.lamp2_current;
    jsonDoc["Voltage_24V"] = systemStatus.ic24V_voltage;
    jsonDoc["Chip_Temp_24V"] = systemStatus.ic24V_temperature;
    jsonDoc["24V_0_State"] = systemStatus.ic24V_C0_state;
    jsonDoc["24V_1_State"] = systemStatus.ic24V_C1_state;
    jsonDoc["Current_BD3D"] = systemStatus.bd3d_current;
    jsonDoc["Current_IPCam"] = systemStatus.ipcam_current;
    jsonDoc["Voltage_12V"] = systemStatus.ic12V_voltage;
    jsonDoc["Chip_Temp_12V"] = systemStatus.ic12V_temperature;
    jsonDoc["12V_0_State"] = systemStatus.ic12V_C0_state;
    jsonDoc["12V_1_State"] = systemStatus.ic12V_C1_state;
    serializeJson(jsonDoc, jsonString);
}

/* COMANDI PER ESECUZIONE SINCRONA */

/*
* Funzione che imposta lo stato dei componenti
*/
void setCommand(String component, String action){
    if (component == "ipcam"){
        //write485("Comando ricevuto: IPcam ");
        blinkDebugLED(LED_DEBUG_GREEN);
        if (action == "on"){
            //write485("on");
            VND70::channel_0(1, true);
        } else if (action == "off"){
            //write485("off");
            VND70::channel_0(1, false);
        }
        
    } else if (component == "bd3d"){
        //write485("Comando ricevuto: BlueDepth ");
        blinkDebugLED(LED_DEBUG_GREEN);
        if (action == "on"){
            //write485("on");
            VND70::channel_1(1, true);
        } else if (action == "off"){
            //write485("off");
            VND70::channel_1(1, false);
        }

    } else if (component == "lampsx"){
        //write485("Comando ricevuto: LampSX ");
        blinkDebugLED(LED_DEBUG_GREEN);
        if (action == "on"){
            //write485("on");
            VND70::channel_0(2, true);
        } else if (action == "off"){
            //write485("off");
            VND70::channel_0(2, false);
        }

    } else if (component == "lampdx"){
        //write485("Comando ricevuto: LampDX ");
        blinkDebugLED(LED_DEBUG_GREEN);
        if (action == "on"){
            //write485("on");
            VND70::channel_1(2, true);
        } else if (action == "off"){
            //write485("off");
            VND70::channel_1(2, false);
        }

    } else if (component == "lamp"){
        //write485("Comando ricevuto: Lamp ");
        blinkDebugLED(LED_DEBUG_GREEN);
        if (action == "reset") {
            resetLamp();
        } else if (action == "standby") {
            write485LampSX("\nflash " + action + "\n\r");            // Invio del comando di standby ai flash tramite 485
            write485LampDX("\nflash " + action + "\n\r");            // Invio del comando di standby ai flash tramite 485
        } else if (action == "idle") {
            write485LampSX("\nflash " + action + "\n\r");            // Invio del comando di idle ai flash tramite 485
            write485LampDX("\nflash " + action + "\n\r");            // Invio del comando di idle ai flash tramite 485
        } else if (action == "flash") {
            write485LampSX("\nflash " + action + "\n\r");            // Invio del comando di flash ai flash tramite 485
            write485LampDX("\nflash " + action + "\n\r");            // Invio del comando di flash ai flash tramite 485
        }

    } else if (component == "lamp_power") {
        //write485("Comando ricevuto: Lamp_power ");
        blinkDebugLED(LED_DEBUG_GREEN);
        int power = action.toInt();
        if (power >= 0 && power <= MAX_POWER_FLASH) {
            write485LampSX("power_flash " + action  + "\n\r");
            write485LampDX("power_flash " + action  + "\n\r");
        }

    } else if (component == "lamp_torch") {
        //write485("Comando ricevuto: Lamp_power ");
        blinkDebugLED(LED_DEBUG_GREEN);
        int power = action.toInt();
        if (power == 0){
            write485LampSX("torch 0\n\r");
            write485LampDX("torch 0\n\r");
        } else if (power == 1) {
            write485LampSX("torch 1\n\r");
            write485LampDX("torch 1\n\r");
        }

    }  else if (component == "light"){
        //write485("Comando ricevuto: Light ");
        blinkDebugLED(LED_DEBUG_GREEN);
        int pwm_light = action.toInt();
        if (pwm_light >= 0 && pwm_light <= 255){        
            if (pwm_light != 0){
                // write485("on");
                VND70::channel_0(1, true);          // Accendo il canale IPcam e Lights
                ledcWrite(0, pwm_light);
            } else {
                ledcWrite(0, 0);                    // spegni PWM
            }
        } else {
            write485ROV("PWM light out of pwm range\n\r");
        }
    }
}

void ID_setCommand(char id_num[ID_NUM_SIZE]){
    if (isLetter(id_num[0]) && isLetter(id_num[1]) && isLetter(id_num[2])){ // Verifica che i primi 3 caratteri siano lettere
        writeTelnet("# Comando ricevuto: ID_SET " + String(id_num));          // Stampa l'ID ricevuto
        EEPROM.writeBytes(ADDR_ID_NUM, id_num, ID_NUM_SIZE);                // Salva l'ID nella EEPROM
        EEPROM.commit();
        EEPROM.readBytes(ADDR_ID_NUM, id_num, ID_NUM_SIZE);     // Legge l'ID salvato nella EEPROM
        writeTelnet("# ID_NUM salvato: " + String(id_num));
    }
    else {
        writeTelnet("ID non valido");
    }
}

void ID_printCommand(){
    char id_num[ID_NUM_SIZE];
    EEPROM.readBytes(ADDR_ID_NUM, id_num, ID_NUM_SIZE);     // Legge l'ID salvato nella EEPROM
    writeTelnet("ID del dispositivo: " + String(id_num));   // Stampa l'ID del dispositivo
}

/*------------OTA UPDATE------------*/

void notFound(AsyncWebServerRequest *request)
{
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

/*---------SERVER INITIALIZE--------*/
void server_initialize(){
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", MAIN_page);});
        //request->send(200, "text/plain", "HUB MPE");});
    
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

/* --------------CLI-------------- */

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
    delay(2000);
    VND70::channel_0(1, true);              // Temporizzo le accensioni per evitare assorbimenti elevati
    delay(1000);
    VND70::channel_1(1, true);
    delay(1000);
    VND70::channel_0(2, true);
    delay(1000);
    VND70::channel_1(2, true);
    delay(1000);
    set_pin_function(OUTPUT_ARRAY, sizeof(OUTPUT_ARRAY), LOW);  // Porto a LOW tutte le uscite
    tone(BUZZER_DEBUG, 600, 100);
    tone(BUZZER_DEBUG, 1200, 200);
}

void loop() {
    #ifdef DEBUG
        scanI2C();
        print_ADC();
    #endif
    delay(5);
    ElegantOTA.loop();
    String input = loopTelnet();

    jsonSerialize();

    // Verifico che la lampada SX rispondano ogni "LAMP_COMMUNICATION_TIMEOUT" millisecondi (se la lampada è accesa)
    if (millis() - systemStatus.last_lampSX_comm_time > LAMP_COMMUNICATION_TIMEOUT && systemStatus.ic24V_C0_state) {
        systemStatus.lampSX_isAlive = false;
        write485ROV("ERROR: LampSX not responding\n\r");
        writeTelnet("ERROR: LampSX not responding");
        systemStatus.last_lampSX_comm_time = millis(); // Evito di mandare troppi messaggi di errore
    }
    else {
        systemStatus.lampSX_isAlive = true;
    }

    // Verifico che la lampada DX rispondano ogni "LAMP_COMMUNICATION_TIMEOUT" millisecondi (se la lampada è accesa)
    if (millis() - systemStatus.last_lampDX_comm_time > LAMP_COMMUNICATION_TIMEOUT && systemStatus.ic24V_C1_state) {
        systemStatus.lampDX_isAlive = false;
        write485ROV("ERROR: LampDX not responding\n\r");
        writeTelnet("ERROR: LampDX not responding");
        systemStatus.last_lampDX_comm_time = millis(); // Evito di mandare troppi messaggi di errore
    }
    else {
        systemStatus.lampDX_isAlive = true;
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

    if (SerialROV.available() > 3){
        String response = SerialROV.readStringUntil('\n'); // Leggo la risposta dal 485
        if (response[0] != ' ' && response[0] != '#')
            writeTelnet("#485ROV: " + response);          // Invia la risposta al client telnet
    } else {
        while (SerialROV.available())
            SerialROV.read();
    }

    if (SerialLampSX.available() > 3){
        String response = SerialLampSX.readStringUntil('\n'); // Leggo la risposta dal 485
        systemStatus.last_lampSX_comm_time = millis();
        if (response[0] != ' ' && response[0] != '#')
            writeTelnet("#485LampSX: " + response);          // Invia la risposta al client telnet
    } else {
        while (SerialLampSX.available())
            SerialLampSX.read();
    }

    if (SerialLampDX.available() > 3){
        String response = SerialLampDX.readStringUntil('\n'); // Leggo la risposta dal 485
        systemStatus.last_lampDX_comm_time = millis();
        if (response[0] != ' ' && response[0] != '#')
            writeTelnet("#485LampDX: " + response);          // Invia la risposta al client telnet
    } else {
        while (SerialLampDX.available())
            SerialLampDX.read();
    }

}
