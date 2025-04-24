#include <HUB_firmware.h>
#include <VND70.h>

/*------------COMANDI CLI------------*/
SimpleCLI cli;                                  // Oggetto per CLI
Command ping;
Command set;

void setup() {
    declaration_function(OUTPUT_ARRAY, sizeof(OUTPUT_ARRAY), OUTPUT);
    declaration_function(INPUT_ARRAY, sizeof(INPUT_ARRAY), INPUT);

    Serial2.begin(9600, RX_485, TX_485);    // begin RS485
    Serial.begin(9600);                     // begin porta seriale USB
    while (!Serial) { delay(10); }          // attendo inizializzazione seriale

    //initialize();
    /* Setup e verifica comandi CLI */
    set = cli.addCmd("set");
    set.addArgument("state");
    set.addPositionalArgument("power","1");
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
    VND70::registerComponent(1, MULTISENSE_24V, ENABLE_0_24V, ENABLE_1_24V, ENABLE_SENS_24V, SEL_0_24V, SEL_1_24V);  // ID=1
    VND70::registerComponent(2, MULTISENSE_12V, ENABLE_0_12V, ENABLE_1_12V, ENABLE_SENS_12V, SEL_0_12V, SEL_1_12V);  // ID=2
    VND70::begin();
}

void loop() {
    /*
        // Esegui azione 1 sul componente 1
        VND70::action1(1);
        delay(1000);

        // Esegui azione 2 sul componente 2
        VND70::action2(2);
        delay(1000);
    */
}
