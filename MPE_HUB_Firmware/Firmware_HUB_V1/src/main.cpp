#include <HUB_firmware.h>
#include <VND70.h>
#include <AsyncWebServer_ESP32_W5500.h>
#include <ElegantOTA.h>

/*------------COMANDI CLI------------*/
SimpleCLI cli;                                  // Oggetto per CLI
Command ping;
Command set;
Command standby;
Command help;

/*------------OTA UPDATE------------*/

const char* ssid = "HUB_MPE";
const char* password = "00000000";

AsyncWebServer server(80);

unsigned long ota_progress_millis = 0;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01 };

// Select the IP address according to your local network
IPAddress myIP(192, 168, 0, 232);
IPAddress myGW(192, 168, 0, 1);
IPAddress mySN(255, 255, 255, 0);
// Google DNS Server IP
IPAddress myDNS(8, 8, 8, 8);

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
        request->send(200, "text/plain", "Flash light BD3D");});
    
    // Definizione dell'endpoint GET su /dati
    server.on("/dati", HTTP_GET, [](AsyncWebServerRequest *request){
        String jsonString;
        DynamicJsonDocument jsonDoc(2048);                  // Creazione del json

        jsonDoc["Current Flash 1"] = VND70::readCurrent(2, FLASH_1_CHANNEL);
        jsonDoc["Current Flash 2"] = VND70::readCurrent(2, FLASH_2_CHANNEL);
        //jsonDoc["Voltage 24V"] = VND70::readVoltage(2);
        jsonDoc["Chip Temp 24V"] = 23.5;
        jsonDoc["Current BD3D"] = VND70::readCurrent(1, BD3D_CHANNEL);
        jsonDoc["Current IPCam"] = VND70::readCurrent(1, IPCAM_CHANNEL);
        //jsonDoc["Voltage 12V"] = VND70::readVoltage(1);
        jsonDoc["Chip Temp 12V"] = 23.5;

        serializeJson(jsonDoc, jsonString);
        request->send(200, "application/json", jsonString); // Invio della risposta
    });

    server.on("/lamp", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("state")) {
            String Status = request->getParam("state")->value();
            cli.parse("set Lamp " + Status);
            request->send(200, "text/plain", "Lamp set to: " + Status);
        } else {
            request->send(400, "text/plain", "Missing 'state' parameter");
        }
    });

    server.on("/lamp/power", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("state") && request->hasParam("power")) {
            String Status = request->getParam("state")->value();
            String Power = request->getParam("power")->value();
            write485("flash standby 2");
            request->send(200, "text/plain", "Lamp set to: " + Status + Power);
        } else {
            request->send(400, "text/plain", "Missing 'state' or 'power' parameter");
        }
    });

    server.on("/IPCam", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("state")) {
            String Status = request->getParam("state")->value();
            cli.parse("set IPCam " + Status);
            request->send(200, "text/plain", "IPCam set to: " + Status);
        } else {
            request->send(400, "text/plain", "Missing 'state' parameter");
        }
    });

    server.on("/BD3D", HTTP_POST, [](AsyncWebServerRequest *request){
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

/*--------------SETUP--------------*/

void setup() {
    initialize();
    declaration_function(OUTPUT_ARRAY, sizeof(OUTPUT_ARRAY), OUTPUT);
    declaration_function(INPUT_ARRAY, sizeof(INPUT_ARRAY), INPUT);

    // Istanze dei due VND70 
    VND70::registerComponent(1, MULTISENSE_12V, ENABLE_0_12V, ENABLE_1_12V, ENABLE_SENS_12V, SEL_0_12V, SEL_1_12V);  // ID=1
    VND70::registerComponent(2, MULTISENSE_24V, ENABLE_0_24V, ENABLE_1_24V, ENABLE_SENS_24V, SEL_0_24V, SEL_1_24V);  // ID=2
    VND70::begin();

    Serial.print(F("\nStart AsyncSimpleServer_ESP32_W5500 on "));
    Serial.print(ARDUINO_BOARD);
    Serial.print(F(" with "));
    Serial.println(SHIELD_TYPE);
    Serial.println(ASYNC_WEBSERVER_ESP32_W5500_VERSION);

    AWS_LOGWARN(F("Default SPI pinout:"));
    AWS_LOGWARN1(F("SPI_HOST:"), ETH_SPI_HOST);
    AWS_LOGWARN1(F("MOSI:"), MOSI_GPIO);
    AWS_LOGWARN1(F("MISO:"), MISO_GPIO);
    AWS_LOGWARN1(F("SCK:"),  SCK_GPIO);
    AWS_LOGWARN1(F("CS:"),   CS_GPIO);
    AWS_LOGWARN1(F("INT:"),  INT_GPIO);
    AWS_LOGWARN1(F("SPI Clock (MHz):"), SPI_CLOCK_MHZ);
    AWS_LOGWARN(F("========================="));

    // To be called before ETH.begin()
    ESP32_W5500_onEvent();

    // start the ethernet connection and the server:
    // Use DHCP dynamic IP and random mac
    uint16_t index = millis() % NUMBER_OF_MAC;

    delay(1000);

    ETH.begin( MISO_GPIO, MOSI_GPIO, SCK_GPIO, CS_GPIO, INT_GPIO, SPI_CLOCK_MHZ, ETH_SPI_HOST, mac);
    //ETH.begin( MISO_GPIO, MOSI_GPIO, SCK_GPIO, CS_GPIO, INT_GPIO, SPI_CLOCK_MHZ, ETH_SPI_HOST, mac );
    //ETH.config(myIP, myGW, mySN, myDNS);
    ESP32_W5500_waitForConnect();

    server_initialize();

    ElegantOTA.begin(&server);                              // Start ElegantOTA
    ElegantOTA.onStart(onOTAStart);
    ElegantOTA.onProgress(onOTAProgress);
    ElegantOTA.onEnd(onOTAEnd);
    server.begin();
    Serial.print("HTTP server started with IP:");
    Serial.println(ETH.localIP());

    /* Setup e verifica comandi CLI */
    set = cli.addCmd("set");
    set.addPositionalArgument("component");
    set.addPositionalArgument("action");
    set.setDescription("Esegui una determianta \'action\' (on - off) su uno specifico \'component\' (IPcam - BD3D - Lamp)");

    standby = cli.addCmd("standby");
    standby.setDescription("Porta allo stato standby tutto il sistema");

    help = cli.addCommand("help");
    help.setDescription("Panoramica dei comandi");

    ping = cli.addCmd("ping");
    ping.setDescription("Comando di test");
    if (!ping) {
        Serial.println("Something went wrong :(");
    } else {
        Serial.println("Ping was added to the CLI!");
    }

    digitalWrite(LED_DEBUG_RED, HIGH);      // RED
    digitalWrite(LED_DEBUG_GREEN, HIGH);    // GREEN
    tone(BUZZER_DEBUG, 600, 50);
    tone(BUZZER_DEBUG, 300, 100);
    delay(1000);
    set_pin_function(OUTPUT_ARRAY, sizeof(OUTPUT_ARRAY), LOW);  // Apro tutti gli interruttori
    digitalWrite(RST_SWITCH, HIGH);         // Disabilito il reset dello switch (Attivo basso)

}

void loop() {

    ElegantOTA.loop();

    /*  write test
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        Serial.println("# " + input);                   // genera eco
        digitalWrite(RW_485,HIGH);
        delay(10);
        Serial2.print(input + "\n");                         // genera eco
        delay(10);
        tone(BUZZER_DEBUG, 1000, 20);
        cli.parse(input);                               // manda l'input alla CLI
        tone(BUZZER_DEBUG, 1000, 20);
    }
    */
    
    if (Serial2.available()) {
        String input = Serial2.readStringUntil('\n');
        //write485("# " + input);                         // genera eco
        cli.parse(input);                               // manda l'input alla CLI
    }

    /* LISTA COMANDI */
    if (cli.available()) {                              // verifica se ci sono comandi da analizzare
        Command cmd = cli.getCmd();                     // istanzia il comando alla variabile
        if (cmd == ping) {                              // comando di test
            write485("Pong!");
        } else if (cmd == standby) {                    // comando standby
            write485("Comando ricevuto: Stand-by");
            VND70::standby(1);
            VND70::standby(2);
        } else if (cmd == set) {                        // comando set
            Argument compArg = cmd.getArgument("component");
            String compValue = compArg.getValue();
            Argument actionArg = cmd.getArgument("action");
            String actionValue = actionArg.getValue();
            actionValue.toLowerCase();
            if (compValue == "IPcam"){
                write485("Comando ricevuto: IPcam ");
                digitalWrite(LED_DEBUG_GREEN, HIGH);
                delay(20);
                digitalWrite(LED_DEBUG_GREEN, LOW);
                if (actionValue == "on"){
                    write485("on");
                    VND70::channel_0(1, true);
                } else if (actionValue == "off"){
                    write485("off");
                    VND70::channel_0(1, false);
                }
            } else if (compValue == "BD3D"){
                write485("Comando ricevuto: BlueDepth ");
                digitalWrite(LED_DEBUG_GREEN, HIGH);
                delay(20);
                digitalWrite(LED_DEBUG_GREEN, LOW);
                if (actionValue == "on"){
                    write485("on");
                    VND70::channel_1(1, true);
                } else if (actionValue == "off"){
                    write485("off");
                    VND70::channel_1(1, false);
                }
            } else if (compValue == "Lamp"){
                write485("Comando ricevuto: Lamp ");
                digitalWrite(LED_DEBUG_GREEN, HIGH);
                delay(20);
                digitalWrite(LED_DEBUG_GREEN, LOW);
                if (actionValue == "on"){
                    write485("on");
                    VND70::channel_0(2, true);
                    VND70::channel_1(2, true);
                } else if (actionValue == "off"){
                    write485("off");
                    VND70::channel_0(2, false);
                    VND70::channel_1(2, false);
                }
            } else if (compValue == "Light"){
                write485("Comando ricevuto: Light ");
                digitalWrite(LED_DEBUG_GREEN, HIGH);
                delay(20);
                digitalWrite(LED_DEBUG_GREEN, LOW);
                if (actionValue == "on"){
                    write485("on");
                    VND70::channel_0(1, true);      // Accendo il canale IPcam e Lights
                    analogWrite(PWM_LIGHT, 255);
                } else if (actionValue == "off"){
                    write485("off");
                    analogWrite(PWM_LIGHT, 0);
                }
            }
            // power: attributo da 0 a 15 che verrà aggiunto alla tensione di default del DCDC (16 step di potenza)
            /*Argument powerArg = cmd.getArgument("power");
            int powerValue = powerArg.getValue().toInt();
            if (powerValue <= 15 && powerValue >= 0);*/ 
        } else if (cmd == help) {
            write485("Help");
            write485(cli.toString());
            
        }
        /*
            // Esegui azione 1 sul componente 1
            VND70::action1(1);
            delay(1000);

            // Esegui azione 2 sul componente 2
            VND70::action2(2);
            delay(1000);
        */
    }
}
