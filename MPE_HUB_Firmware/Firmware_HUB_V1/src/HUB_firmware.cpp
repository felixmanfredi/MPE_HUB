#include <HUB_firmware.h>
#include <SPI.h>

/*--------------ISTANZE--------------*/
SPIClass spi = SPIClass(HSPI);

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
    spi.begin(PIN_SCLK, PIN_MISO, PIN_MOSI, PIN_SS); // CLK, MISO, MOSI, SS
    pinMode(PIN_SS, OUTPUT);
    digitalWrite(PIN_SS, HIGH);
    if(Serial)
        Serial.println("SPI inizializzata.");
}

// FUNZIONE CHE STAMPA IL TESTO SUL 485
void write485(String text){
    digitalWrite(RW_485,HIGH);                      // attivazione scrittura 485
    delay(2);
    Serial2.println(text);                          // stampa il testo in 485
    Serial.println(text);                           // stampa il testo sulla seriale
    digitalWrite(RW_485,LOW);                       // attivazione lettura 485
    tone(BUZZER_DEBUG, 1000, 20);
}