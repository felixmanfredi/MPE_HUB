#include <HUB_firmware.h>
#include <VND70.h>

/*------------COMANDI CLI------------*/
SimpleCLI cli;                                  // Oggetto per CLI
Command ping;
Command set;
Command standby;

void setup() {
    declaration_function(OUTPUT_ARRAY, sizeof(OUTPUT_ARRAY), OUTPUT);
    declaration_function(INPUT_ARRAY, sizeof(INPUT_ARRAY), INPUT);

    Serial2.begin(9600, RX_485, TX_485);    // begin RS485
    Serial.begin(9600);                     // begin porta seriale USB
    while (!Serial) { delay(10); }          // attendo inizializzazione seriale

    //initialize();
    /* Setup e verifica comandi CLI */
    set = cli.addCmd("set");
    set.addPositionalArgument("component");
    set.addPositionalArgument("action");
    standby = cli.addCmd("standby");
    ping = cli.addCmd("ping");
    if (!ping) {
        Serial.println("Something went wrong :(");
    } else {
        Serial.println("Ping was added to the CLI!");
    }

    digitalWrite(LED_DEBUG_1, HIGH);    // RED
    digitalWrite(LED_DEBUG_2, HIGH);    // GREEN
    tone(BUZZER_DEBUG, 500, 100);
    delay(1000);
    set_pin_function(OUTPUT_ARRAY, sizeof(OUTPUT_ARRAY), LOW);  // Apro tutti gli interruttori

    // Istanze dei due VND70 
    VND70::registerComponent(1, MULTISENSE_12V, ENABLE_0_12V, ENABLE_1_12V, ENABLE_SENS_12V, SEL_0_12V, SEL_1_12V);  // ID=1
    VND70::registerComponent(2, MULTISENSE_24V, ENABLE_0_24V, ENABLE_1_24V, ENABLE_SENS_24V, SEL_0_24V, SEL_1_24V);  // ID=2
    VND70::begin();
}

void loop() {
    /* LISTA COMANDI */
    if (cli.available()) {                              // verifica se ci sono comandi da analizzare
        Command cmd = cli.getCmd();                     // istanzia il comando alla variabile
        if (cmd == ping) {                              // comando di test
            Serial.println("Pong!");
        } else if (cmd == standby) {                    // comando standby
            Serial.println("Comando ricevuto: Stand-by");
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
                if (actionValue == "on"){
                    Serial.println("on");
                    VND70::channel_0(1, true);
                } else if (actionValue == "off"){
                    Serial.println("off");
                    VND70::channel_0(1, false);
                }
            } else if (compValue == "BD3D"){
                Serial.print("Comando ricevuto: BlueDepth ");
                if (actionValue == "on"){
                    Serial.println("on");
                    VND70::channel_1(1, true);
                } else if (actionValue == "off"){
                    Serial.println("off");
                    VND70::channel_1(1, false);
                }
            } else if (compValue == "Lamp"){
                Serial.print("Comando ricevuto: Lamp ");
                if (actionValue == "on"){
                    Serial.println("on");
                    VND70::channel_0(1, true);
                    VND70::channel_1(2, true);
                } else if (actionValue == "off"){
                    Serial.println("off");
                    VND70::channel_0(1, false);
                    VND70::channel_1(2, false);
                }
            }
            
            // power: attributo da 0 a 15 che verrà aggiunto alla tensione di default del DCDC (16 step di potenza)
            Argument powerArg = cmd.getArgument("power");
            int powerValue = powerArg.getValue().toInt();
            if (powerValue <= 15 && powerValue >= 0);
                
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
