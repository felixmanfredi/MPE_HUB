#include <HUB_firmware.h>
#include <VND70.h>
#include <AsyncWebServer_ESP32_SC_W5500.h>
#include <ElegantOTA.h>

/*------------COMANDI CLI------------*/
SimpleCLI cli;                                  // Oggetto per CLI
Command ping;
Command set;
Command standby;
Command help;

/*------------VARIABILI-------------*/

const char* ssid = "HUB_MPE";
const char* password = "00000000";

IPAddress IP;
AsyncWebServer server(80);

unsigned long ota_progress_millis = 0;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01 };

// Select the IP address according to your local network
IPAddress myIP(192, 168, 1, 231);
IPAddress myGW(192, 168, 1, 1);
IPAddress mySN(255, 255, 255, 0);
// Google DNS Server IP
IPAddress myDNS(8, 8, 8, 8);

String jsonString;
DynamicJsonDocument jsonDoc(2048);                  // Creazione del json

struct flash_wrapper {
    String power_value = "0";
    bool flash_action_flag = false;
    String torch_value = "0";
    bool torch_action_flag = false;
}flash_wrapper;

/*------------FUNZIONI---------------*/

void jsonSerialize(){
    jsonDoc.clear();
    jsonDoc["Current_Flash_1"] = VND70::readCurrent(2, FLASH_1_CHANNEL);
    jsonDoc["Current_Flash_2"] = VND70::readCurrent(2, FLASH_2_CHANNEL);
    jsonDoc["Voltage_24V"] = VND70::readVoltage(2);
    jsonDoc["Chip_Temp_24V"] = VND70::readTemperature(2);
    jsonDoc["Current_BD3D"] = VND70::readCurrent(1, BD3D_CHANNEL);
    jsonDoc["Current_IPCam"] = VND70::readCurrent(1, IPCAM_CHANNEL);
    jsonDoc["Voltage_12V"] = VND70::readVoltage(1);
    jsonDoc["Chip_Temp_12V"] = VND70::readTemperature(1);
    jsonDoc["12V_0_State"] = VND70::channel_0_state(1);
    jsonDoc["12V_1_State"] = VND70::channel_1_state(1);
    jsonDoc["24V_0_State"] = VND70::channel_0_state(2);
    jsonDoc["24V_1_State"] = VND70::channel_1_state(2);
    serializeJson(jsonDoc, jsonString);
}

/* COMANDI PER ESECUZIONE SINCRONA */

/*
* Funzione che imposta lo stato dei componenti
*/
void setCommand(String component, String action){
    if (component == "ipcam"){
        //write485("Comando ricevuto: IPcam ");
        digitalWrite(LED_DEBUG_GREEN, HIGH);
        delay(20);
        digitalWrite(LED_DEBUG_GREEN, LOW);
        if (action == "on"){
            //write485("on");
            VND70::channel_0(1, true);
        } else if (action == "off"){
            //write485("off");
            VND70::channel_0(1, false);
        }
    } else if (component == "bd3d"){
        //write485("Comando ricevuto: BlueDepth ");
        digitalWrite(LED_DEBUG_GREEN, HIGH);
        delay(20);
        digitalWrite(LED_DEBUG_GREEN, LOW);
        if (action == "on"){
            //write485("on");
            VND70::channel_1(1, true);
        } else if (action == "off"){
            //write485("off");
            VND70::channel_1(1, false);
        }
    } else if (component == "lamp"){
        //write485("Comando ricevuto: Lamp ");
        digitalWrite(LED_DEBUG_GREEN, HIGH);
        delay(20);
        digitalWrite(LED_DEBUG_GREEN, LOW);
        if (action == "on"){
            //write485("on");
            VND70::channel_0(2, true);
            VND70::channel_1(2, true);
        } else if (action == "off"){
            //write485("off");
            VND70::channel_0(2, false);
            VND70::channel_1(2, false);
        } else if (action == "reset") {
            write485(action);                                // Invio del comando di reset ai flash tramite 485
        } else if (action == "standby") {
            write485("\nflash " + action + "\n");            // Invio del comando di standby ai flash tramite 485
        } else if (action == "idle") {
            write485("\nflash " + action + "\n");            // Invio del comando di idle ai flash tramite 485
        } else if (action == "torch") {
            write485("\nflash " + action + "\n");            // Invio del comando di torch ai flash tramite 485
        } else if (action == "flash") {
            write485("\nflash " + action + "\n");            // Invio del comando di flash ai flash tramite 485
        }
    } else if (component == "lamp_power") {
        //write485("Comando ricevuto: Lamp_power ");
        digitalWrite(LED_DEBUG_GREEN, HIGH);
        delay(20);
        digitalWrite(LED_DEBUG_GREEN, LOW);
        int power = action.toInt();
        if (power >= 0 && power <= MAX_POWER_FLASH) {
            write485("power_flash " + action  + "\n\r");
        }

    } else if (component == "lamp_torch") {
        //write485("Comando ricevuto: Lamp_power ");
        digitalWrite(LED_DEBUG_GREEN, HIGH);
        delay(20);
        digitalWrite(LED_DEBUG_GREEN, LOW);
        int power = action.toInt();
        if (power >= 0 && power <= 3) {
            write485("torch " + action  + "\n\r");
        }

    }  else if (component == "light"){
        //write485("Comando ricevuto: Light ");
        digitalWrite(LED_DEBUG_GREEN, HIGH);
        delay(20);
        digitalWrite(LED_DEBUG_GREEN, LOW);
        int pwm_light = action.toInt();
        if (pwm_light >= 0 && pwm_light <= 255){        
            if (pwm_light != 0){
                // write485("on");
                VND70::channel_0(1, true);          // Accendo il canale IPcam e Lights
                analogWrite(PWM_LIGHT, pwm_light);
            } else {
                //write485("off");
                analogWrite(PWM_LIGHT, 0);
            }
        } else {
            write485("PWM light out of pwm range\n\r");
        }
    }
}



/*------------OTA UPDATE------------*/

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

void onOTAStart() {
    // Log when OTA has started
    Serial.println("OTA update started!");
    // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
    // Log every 1 second
    if (millis() - ota_progress_millis > 1000) {
        ota_progress_millis = millis();
        Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
    }
}

void onOTAEnd(bool success) {
    // Log when OTA has finished
    if (success) {
        Serial.println("OTA update finished successfully!");
    } else {
        Serial.println("There was an error during OTA update!");
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
            request->send(200, "text/plain", "Lamp power set to: " + Power);
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
        write485("reset\n");                                                    // Invio del comando di reset ai flash tramite 485
        if(Serial2.available()) {
            String response = Serial2.readStringUntil('\n');                    // Leggo la risposta dal seriale
            request->send(200, "text/plain", "Lamp reset error sent.\n Lamp response: " + response);
        } else {
            request->send(200, "text/plain", "Lamp reset error sent.\n No response.");
        }
    });

    server.on("/lamp", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("state")) {
            String Status = request->getParam("state")->value();
            cli.parse("set Lamp " + Status);
            request->send(200, "text/plain", "Lamp set to: " + Status);
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

void helpCallback(cmd* c){
    writeTelnet("Help: " + cli.toString() + "\n");
    /*write485("Help: ");
    write485(cli.toString() + "\n");*/
}

// Callback per gli errori della CLI
void errorCallback(cmd_error* e) {
    CommandError cmdError(e); // Create wrapper object

    Serial.print("ERROR: ");
    Serial.println(cmdError.toString());

    if (cmdError.hasCommand()) {
        Serial.print("Did you mean \"");
        Serial.print(cmdError.getCommand().toString());
        Serial.println("\"?");
    }
}


/*--------------SETUP--------------*/

void setup() {
    initialize();

    // Istanze dei due VND70 
    VND70::registerComponent(1, MULTISENSE_12V, ENABLE_0_12V, ENABLE_1_12V, ENABLE_SENS_12V, SEL_0_12V, SEL_1_12V);  // ID=1
    VND70::registerComponent(2, MULTISENSE_24V, ENABLE_0_24V, ENABLE_1_24V, ENABLE_SENS_24V, SEL_0_24V, SEL_1_24V);  // ID=2
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
    Serial.print("AP IP address: ");
    Serial.println(IP);

    server_initialize();

    ElegantOTA.begin(&server);                              // Start ElegantOTA
    ElegantOTA.onStart(onOTAStart);
    ElegantOTA.onProgress(onOTAProgress);
    ElegantOTA.onEnd(onOTAEnd);
    server.begin();
    Serial.print("HTTP server started with IP:");
    Serial.println(ETH.localIP());

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
    digitalWrite(RST_SWITCH, HIGH);         // Disabilito il reset dello switch (Attivo basso)
    tone(BUZZER_DEBUG, 600, 100);
    tone(BUZZER_DEBUG, 1200, 200);
}

void loop() {
    delay(5);
    ElegantOTA.loop();
    String input = loopTelnet();

    jsonSerialize();

    if (flash_wrapper.flash_action_flag) {
        flash_wrapper.flash_action_flag = false;
        setCommand("lamp_power", flash_wrapper.power_value); // Imposta lo stato dell'illuminatore
    }
    if (flash_wrapper.torch_action_flag) {
        flash_wrapper.torch_action_flag = false;
        setCommand("lamp_torch", flash_wrapper.torch_value); // Imposta lo stato dell'illuminatore
    }


    if (input != "") {
        //write485("# " + input + "\n\r");                  // genera eco su seriale e 485
        cli.parse(input);                                   // manda l'input alla CLI
    }

    if (Serial2.available() > 3){
        String response = Serial2.readStringUntil('\n'); // Leggo la risposta dal 485
        if (response[0] != ' ')
            writeTelnet("#485: " + response);          // Invia la risposta al client telnet
    } else {
        while (Serial2.available())
            Serial2.read();
    }
}
