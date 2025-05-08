#ifndef HUB_FIRMWARE_H
#define HUB_FIRMWARE_H

#include <Arduino.h>
#include <SimpleCLI.h>          // Include per includere interfaccia a linea di comando

#define PIN_MOSI            7
#define PIN_MISO            8
#define PIN_SCLK            9
#define PIN_SS              10

#define TX_485              2   // Pin di tx per 485
#define RW_485              3   // Pin lettura/scrittura per 485 (LOW->READ) (HIGH->WRITE)
#define RX_485              4   // Pin di rx per 485

#define LED_DEBUG_1         37  // RED      // Pin di controllo del led di debug sulla scheda 
#define LED_DEBUG_2         36  // GREEN    // Pin di controllo del led di debug sulla scheda
#define BUZZER_DEBUG        35  // Pin di controllo del buzzer di debug sulla scheda

#define RST_SWITCH          38  // Pin per il reset dello switch di rete (Attivo basso)
#define PWM_LIGHT           39  // Pin PWM per il controllo dei LED del ROV (Luci di posizione)

#define MULTISENSE_24V      12  // Pin lettura analogica sensing (tensione, corrente, temperatura in funzione del mux)
#define ENABLE_0_24V        13  // Pin attivazione canale 0 24V
#define ENABLE_1_24V        17  // Pin attivazione canale 1 24V
#define ENABLE_SENS_24V     14  // Pin attivazione sensing VND70 24V [Active HIGH]
#define SEL_0_24V           15  // Sense MUX 0 24V [Active HIGH]
#define SEL_1_24V           16  // Sense MUX 1 24V [Active HIGH]

#define MULTISENSE_12V      48  // Pin lettura analogica sensing (tensione, corrente, temperatura in funzione del mux)
#define ENABLE_0_12V        21  // Pin attivazione canale 0 12V
#define ENABLE_1_12V        34  // Pin attivazione canale 1 12V
#define ENABLE_SENS_12V     26  // Pin attivazione sensing VND70 12V [Active HIGH]
#define SEL_0_12V           47  // Sense MUX 0 12V [Active HIGH]
#define SEL_1_12V           33  // Sense MUX 1 12V [Active HIGH]

// Array per la dichiarazione degli output (tutti questi controlli sono attivi alti)
const uint8_t OUTPUT_ARRAY[]={TX_485, RW_485, LED_DEBUG_1, LED_DEBUG_2, BUZZER_DEBUG, RST_SWITCH, PWM_LIGHT};

// Array per la dichiarazione degli input
const uint8_t INPUT_ARRAY[]={RX_485};

/*--------------FUNZIONI-------------*/

/*
* FUNZIONE PER INIZIALIZZARE I PIN CONTENUTI IN UN ARRAY
* @param uint8_t array contenete i pin da inizializzare
* @param uint8_t dimensione dell'array [sizeof(array_da_inizializzare)]
* @param byte tipo del pin (INPUT, OUTPUT)
*/
void declaration_function(const uint8_t array[], uint8_t size, byte type);

/*
* FUNZIONE PER IMPOSTARE IL VALORE DEI PIN CONTENUTI IN UN ARRAY
* @param uint8_t array contenete i pin da impostare
* @param uint8_t dimensione dell'array [sizeof(array_da_impostare)]
* @param byte valore del pin (HIGH, LOW)
*/
void set_pin_function(const uint8_t array[], uint8_t size, byte value);

/*
* FUNZIONE PER L'INIZIALIZZAZIONE DI TUTTE LE ISTANZE
* Inizializzazione di: SPI
*
*/
void initialize();

#endif