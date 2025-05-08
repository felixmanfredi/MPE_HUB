#include <HUB_firmware.h>
#include <VND70.h>

/*------------COMANDI CLI------------*/
SimpleCLI cli;                                  // Oggetto per CLI
Command ping;
Command set;
Command standby;
Command help;

void setup() {
    declaration_function(OUTPUT_ARRAY, sizeof(OUTPUT_ARRAY), OUTPUT);
    declaration_function(INPUT_ARRAY, sizeof(INPUT_ARRAY), INPUT);

    // Istanze dei due VND70 
    VND70::registerComponent(1, MULTISENSE_12V, ENABLE_0_12V, ENABLE_1_12V, ENABLE_SENS_12V, SEL_0_12V, SEL_1_12V);  // ID=1
    VND70::registerComponent(2, MULTISENSE_24V, ENABLE_0_24V, ENABLE_1_24V, ENABLE_SENS_24V, SEL_0_24V, SEL_1_24V);  // ID=2
    VND70::begin();

    Serial2.begin(115200, SERIAL_8N1, RX_485, TX_485);    // begin RS485
    Serial.begin(9600);                     // begin porta seriale USB
    //while (!Serial) { delay(10); }          // attendo inizializzazione seriale

    //initialize();
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

    digitalWrite(LED_DEBUG_1, HIGH);    // RED
    digitalWrite(LED_DEBUG_2, HIGH);    // GREEN
    tone(BUZZER_DEBUG, 300, 100);
    tone(BUZZER_DEBUG, 600, 50);
    delay(1000);
    set_pin_function(OUTPUT_ARRAY, sizeof(OUTPUT_ARRAY), LOW);  // Apro tutti gli interruttori
    digitalWrite(RST_SWITCH, HIGH);    // Disabilito il reset dello switch (Attivo basso)

}

void loop() {
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
        digitalWrite(RW_485,HIGH);
        delay(10);
        Serial.println("# " + input);                   // genera eco seriale
        Serial2.println("# " + input);                  // genera eco 485
        cli.parse(input);                               // manda l'input alla CLI
        tone(BUZZER_DEBUG, 1000, 20);
        digitalWrite(RW_485,LOW);
    }

    /* LISTA COMANDI */
    if (cli.available()) {                              // verifica se ci sono comandi da analizzare
        digitalWrite(RW_485,HIGH);                      // imposto il 485 in writing
        delay(10);
        Command cmd = cli.getCmd();                     // istanzia il comando alla variabile
        if (cmd == ping) {                              // comando di test
            Serial.println("Pong!");
            Serial2.println("Pong!");
        } else if (cmd == standby) {                    // comando standby
            Serial.println("Comando ricevuto: Stand-by");
            Serial2.println("Comando ricevuto: Stand-by");
            VND70::standby(1);
            VND70::standby(2);
        } else if (cmd == set) {                        // comando set
            Argument compArg = cmd.getArgument("component");
            String compValue = compArg.getValue();
            Argument actionArg = cmd.getArgument("action");
            String actionValue = actionArg.getValue();
            actionValue.toLowerCase();
            if (compValue == "IPcam"){
                Serial.print("Comando ricevuto: IPcam ");
                Serial2.print("Comando ricevuto: IPcam ");
                digitalWrite(LED_DEBUG_2, HIGH);
                delay(20);
                digitalWrite(LED_DEBUG_2, LOW);
                if (actionValue == "on"){
                    Serial.println("on");
                    Serial2.println("on");
                    VND70::channel_0(1, true);
                } else if (actionValue == "off"){
                    Serial.println("off");
                    Serial2.println("off");
                    VND70::channel_0(1, false);
                }
            } else if (compValue == "BD3D"){
                Serial.print("Comando ricevuto: BlueDepth ");
                Serial2.print("Comando ricevuto: BlueDepth ");
                digitalWrite(LED_DEBUG_2, HIGH);
                delay(20);
                digitalWrite(LED_DEBUG_2, LOW);
                if (actionValue == "on"){
                    Serial.println("on");
                    Serial2.println("on");
                    VND70::channel_1(1, true);
                } else if (actionValue == "off"){
                    Serial.println("off");
                    Serial2.println("off");
                    VND70::channel_1(1, false);
                }
            } else if (compValue == "Lamp"){
                Serial.print("Comando ricevuto: Lamp ");
                Serial2.print("Comando ricevuto: Lamp ");
                digitalWrite(LED_DEBUG_2, HIGH);
                delay(20);
                digitalWrite(LED_DEBUG_2, LOW);
                if (actionValue == "on"){
                    Serial.println("on");
                    Serial2.println("on");
                    VND70::channel_0(2, true);
                    VND70::channel_1(2, true);
                } else if (actionValue == "off"){
                    Serial.println("off");
                    Serial2.println("off");
                    VND70::channel_0(2, false);
                    VND70::channel_1(2, false);
                }
            } else if (compValue == "Light"){
                Serial.print("Comando ricevuto: Light ");
                Serial2.print("Comando ricevuto: Light ");
                digitalWrite(LED_DEBUG_2, HIGH);
                delay(20);
                digitalWrite(LED_DEBUG_2, LOW);
                if (actionValue == "on"){
                    Serial.println("on");
                    Serial2.println("on");
                    VND70::channel_0(1, true);      // Accendo il canale IPcam e Lights
                    analogWrite(PWM_LIGHT, 255);
                } else if (actionValue == "off"){
                    Serial.println("off");
                    Serial2.println("off");
                    analogWrite(PWM_LIGHT, 0);
                }
            }
            // power: attributo da 0 a 15 che verrà aggiunto alla tensione di default del DCDC (16 step di potenza)
            /*Argument powerArg = cmd.getArgument("power");
            int powerValue = powerArg.getValue().toInt();
            if (powerValue <= 15 && powerValue >= 0);*/ 
        } else if (cmd == help) {
            Serial.println("Help:");
            Serial2.println("Help:");
            Serial2.println(cli.toString());
            Serial.println(cli.toString());
            
        }
        /*
            // Esegui azione 1 sul componente 1
            VND70::action1(1);
            delay(1000);

            // Esegui azione 2 sul componente 2
            VND70::action2(2);
            delay(1000);
        */
        digitalWrite(RW_485,LOW);                      // imposto il 485 in reading
    }
}
