#ifndef HUB_FIRMWARE_H
#define HUB_FIRMWARE_H

#include <Arduino.h>
#include <Wire.h>
#include <SimpleCLI.h>          // Include per includere interfaccia a linea di comando
#include <ArduinoJson.h>        // Include per la creazione di json
#include <SPI.h>
#include <Adafruit_ADS1X15.h>   // Include per l'ADC I2C
#include <EEPROM.h>             // Include library to read and write from flash memory
#include <RS485.h>


//#define DEBUG                 // Abilita le stampe di debug
//#define DEBUG_STATUS          // Abilita la stampa su telnet dello stato del sistema
//#define LOG_DEBUG             // Abilita la stampa dei log su telnet e la stampa dello stato del filesystem
//#define ECO485                // Abilita l'eco dei comandi ricevuti sulla CLI al 485 del ROV

#define SERIAL_SPEED    115200  // baud rate seriale
#define SDA_PIN             34  // Pin SDA per I2C
#define SCL_PIN             48  // Pin SCL per I2C

#define EEPROM_SIZE         0x30    // Il numero di byte da utilizzare della EEPROM
#define FIRST_CYCLE_KEY     0x99    // Un valore che mi funge da chiave per capire se è il primo avvio
#define ADDR_FIRST_CYCLE    0x00    // Indirizzo di scrittura per il controllo del primo avvio
#define ADDR_POWER_CYCLE    0x01    // Indirizzo iniziale di scrittura per il numero di avii della scheda
#define ADDR_ID_NUM         0x10    // Indirizzo iniziale di scrittura ID_NUM e scrive i successivi ID_NUM_SIZE byte
#define ID_NUM_SIZE         17      // Numero di byte dell'ID (16 byte + terminatore di stringa)

#define RST_GPIO            5   // Pin di reset verso W5500
#define INT_GPIO            6   // Pin di interrupt del W5500
#define MISO_GPIO           8   // Pin MISO SPI
#define MOSI_GPIO           7   // Pin MOSI SPI
#define SCK_GPIO            9   // Pin SCK SPI
#define CS_GPIO             10  // Pin CS SPI

#define PRINT_485DELAY_TIME     10  // Millisecondi di attesa per scrivere sui bus 485
#define LAMP_CHECK_STATUS_TIMER 100 // Quanti millisecondi passano tra un controllo delle lampade e il successivo
#define MIN_RX_BYTE_485         3   // Numero minimo di byte da leggere per considerare valido un messaggioo sul bus 485

#define RX_485_LED_SX       2   // Pin di rx per 485 del flash di sinistra (Flash 1 sulla scheda)
#define RW_485_LED_SX       3   // Pin lettura/scrittura per 485 (LOW->READ) (HIGH->WRITE) (Flash 1 sulla scheda)
#define TX_485_LED_SX       4   // Pin di tx per 485 del flash di sinistra (Flash 1 sulla scheda)

#define TX_485_LED_DX       11  // Pin di tx per 485 del flash di destra (Flash 2 sulla scheda)
#define RW_485_LED_DX       12  // Pin lettura/scrittura per 485 (LOW->READ) (HIGH->WRITE) (Flash 2 sulla scheda)
#define RX_485_LED_DX       13  // Pin di rx per 485 del flash di destra (Flash 2 sulla scheda)

#define TX_485_ROV          42  // Pin di tx per 485 del ROV
#define RW_485_ROV          41  // Pin lettura/scrittura per 485 (LOW->READ) (HIGH->WRITE)
#define RX_485_ROV          40  // Pin di rx per 485 del ROV

#define LED_DEBUG_RED       37  // RED      // Pin di controllo del led di debug sulla scheda 
#define LED_DEBUG_GREEN     36  // GREEN    // Pin di controllo del led di debug sulla scheda
#define BUZZER_DEBUG        35  // Pin di controllo del buzzer di debug sulla scheda

#define WATER_PROBE_PIN     38  // Pin per il rilevamento dell'acqua

#define PWM_LIGHT           39  // Pin PWM per il controllo dei LED del ROV (Luci di posizione)

#define MULTISENSE_24V_ADC_PIN  1  // Pin dell'ADC connesso al multisense 24V
#define ENABLE_0_24V        17  // Pin attivazione canale 0 24V
#define ENABLE_1_24V        14  // Pin attivazione canale 1 24V
#define SEL_0_24V           16  // Sense MUX 0 24V [Active HIGH]
#define SEL_1_24V           15  // Sense MUX 1 24V [Active HIGH]
#define FLASH_1_CHANNEL     0   // Canale utilizzato per il flash 1
#define FLASH_2_CHANNEL     1   // Canale utilizzato per il flash 2

#define MULTISENSE_12V_ADC_PIN  0  // Pin dell'ADC connesso al multisense 12V
#define ENABLE_0_12V        21  // Pin attivazione canale 0 12V
#define ENABLE_1_12V        18  // Pin attivazione canale 1 12V
#define SEL_0_12V           47  // Sense MUX 0 12V [Active HIGH]
#define SEL_1_12V           33  // Sense MUX 1 12V [Active HIGH]
#define IPCAM_CHANNEL       0   // Canale utilizzato per l'IPCam
#define BD3D_CHANNEL        1   // Canale utilizzato per la BlueDepth

#define MAX_POWER_FLASH     4   // Massima potenza impostabile del flash
#define MAX_POWER_TORCH     3   // Massima potenza impostabile in modalità torcia

#define _ASYNC_WEBSERVER_LOGLEVEL_       2

#define ADC_ADDR            0x4B// Indirizzo I2C per l'ADC 4 canali
#define R1_ADC_DIVIDER      10  // Valore della prima resistenza del partitore per la lettura da ADC [kOhm]
#define R2_ADC_DIVIDER      20  // Valore della seconda resistenza del partitore (su cui avviene la lettura) [kOhm]
#define RC_ADC_DIVIDER      1   // Valore della prima resistenza del partitore per la lettura da ADC [kOhm]

#define BLINK_DELAY_TIME    10  // Tempo di lampeggio del led di debug in ms

// ---------- Gestione Warning ed errori ---------------
#define PRINT_ERROR_TIMER                   5000    // Intervallo tra una stampa degli errori e la successiva [mS]
#define LAMP_COMMUNICATION_TIMEOUT          6500    // Tempo massimo di attesa per la comunicazione con le lampade in ms
#define LOG_INTERVAL                        900000  // Intervallo di log dei dati di sistema in ms

#define WARNING_UNDERVOLTAGE_12V_THRESHOLD  11  // Soglia di warning undervoltage per l'alimentazione 12V [V]
#define WARNING_OVERVOLTAGE_12V_THRESHOLD   13  // Soglia di warning overvoltage per l'alimentazione 12V [V]
#define WARNING_OVERTEMP_12V_THRESHOLD      60  // Soglia di warning overtemperature per l'alimentazione 12V [°C]
#define WARNING_OVERCURRENT_IPCAM_THRESHOLD 1500 // Soglia di warning overcurrent per l'alimentazione 12V IPCAM [mA]
#define WARNING_OVERCURRENT_BD3D_THRESHOLD  3000 // Soglia di warning overcurrent per l'alimentazione 12V BD3D [mA]

#define WARNING_UNDERVOLTAGE_24V_THRESHOLD  22  // Soglia di warning undervoltage per l'alimentazione 24V [V]
#define WARNING_OVERVOLTAGE_24V_THRESHOLD   26  // Soglia di warning overvoltage per l'alimentazione 24V [V]
#define WARNING_OVERTEMP_24V_THRESHOLD      60  // Soglia di warning overtemperature per l'alimentazione 24V [°C]
#define WARNING_OVERCURRENT_LAMPSX_THRESHOLD  2500 // Soglia di warning overcurrent per l'alimentazione 24V Lampada SX [mA]
#define WARNING_OVERCURRENT_LAMPDX_THRESHOLD  2500 // Soglia di warning overcurrent per l'alimentazione 24V Lampada DX [mA]

#define ERROR_UNDERVOLTAGE_12V_THRESHOLD    5   // Soglia di errore undervoltage per l'alimentazione 12V [V]
#define ERROR_OVERVOLTAGE_12V_THRESHOLD     15  // Soglia di errore overvoltage per l'alimentazione 12V [V]
#define ERROR_OVERTEMP_12V_THRESHOLD        70  // Soglia di errore overtemperature per l'alimentazione 12V [°C]
#define ERROR_OVERCURRENT_IPCAM_THRESHOLD   2000 // Soglia di errore overcurrent per l'alimentazione 12V IPCAM [mA]
#define ERROR_OVERCURRENT_BD3D_THRESHOLD    5000 // Soglia di errore overcurrent per l'alimentazione 12V BD3D [mA]

#define ERROR_UNDERVOLTAGE_24V_THRESHOLD    15  // Soglia di errore undervoltage per l'alimentazione 24V [V]
#define ERROR_OVERVOLTAGE_24V_THRESHOLD     30  // Soglia di errore overvoltage per l'alimentazione 24V [V]
#define ERROR_OVERTEMP_24V_THRESHOLD        70  // Soglia di errore overtemperature per l'alimentazione 24V [°C]
#define ERROR_OVERCURRENT_LAMPSX_THRESHOLD  3500 // Soglia di errore overcurrent per l'alimentazione 24V Lampada SX [mA]
#define ERROR_OVERCURRENT_LAMPDX_THRESHOLD  3500 // Soglia di errore overcurrent per l'alimentazione 24V Lampada DX [mA]

// Warning ENUM
enum WarningCode: uint8_t {
    WARNING_UNDERVOLTAGE_12V,
    WARNING_OVERVOLTAGE_12V,
    WARNING_OVERTEMP_12V,
    WARNING_UNDERVOLTAGE_24V,
    WARNING_OVERVOLTAGE_24V,
    WARNING_OVERTEMP_24V,
    WARNING_OVERCURRENT_IPCAM,
    WARNING_OVERCURRENT_BD3D,
    WARNING_OVERCURRENT_LAMPSX,
    WARNING_OVERCURRENT_LAMPDX,
    WARNING_LAMPSX_DISCONNECTED,
    WARNING_LAMPDX_DISCONNECTED,
    WARNING_LAST_INDEX          // Da lasciare per ultimo per sapere di quanti valori inizializzare l'array di warning
};

// Error ENUM
enum ErrorCode: uint8_t {
    ERROR_UNDERVOLTAGE_12V,
    ERROR_OVERVOLTAGE_12V,
    ERROR_OVERTEMP_12V,
    ERROR_UNDERVOLTAGE_24V,
    ERROR_OVERVOLTAGE_24V,
    ERROR_OVERTEMP_24V,
    ERROR_OVERCURRENT_IPCAM,
    ERROR_OVERCURRENT_BD3D,
    ERROR_OVERCURRENT_LAMPSX,
    ERROR_OVERCURRENT_LAMPDX,
    ERROR_WATER_LEAK_DETECTED,
    ERROR_LAST_INDEX            // Da lasciare per ultimo per sapere di quanti valori inizializzare l'array di errori
};

// Array per la dichiarazione degli output (tutti questi controlli sono attivi alti)
const uint8_t OUTPUT_ARRAY[]={RW_485_LED_SX, RW_485_LED_DX, RW_485_ROV,
                              LED_DEBUG_RED, LED_DEBUG_GREEN, BUZZER_DEBUG};

// Array per la dichiarazione degli input
const uint8_t INPUT_ARRAY[]={WATER_PROBE_PIN};

// Dichiarazione porte seriali per 485 dedicati
extern HardwareSerial SerialROV;
extern HardwareSerial SerialLampSX;
extern HardwareSerial SerialLampDX;

extern RS485Bus ROV485;
extern RS485Bus LampSX485;
extern RS485Bus LampDX485;

// Dichiarazione della struct contenente le variabili di sistema
struct systemStatusStruct {
    char ID[ID_NUM_SIZE] = {0};                         // ID univoco della scheda
    char Board_REV[ID_NUM_SIZE] = "1.1.0";
    char FW_VERS[ID_NUM_SIZE] = "1.3.2";
    uint16_t power_cycle_count = 0;                     // Numero di avvi della scheda
    unsigned long last_lampSX_comm_time = 0;
    unsigned long last_lampDX_comm_time = 0;
    float lamp1_current = 0.0f;
    float lamp2_current = 0.0f;
    float ic24V_voltage = 0.0f;
    float ic24V_temperature = 0.0f;
    bool  ic24V_C0_state = false;
    bool  ic24V_C1_state = false;
    bool  is_torch_mode = false;                       // Indica se le lampade sono in modalità torcia
    float bd3d_current = 0.0f;
    float ipcam_current = 0.0f;
    float ic12V_voltage = 0.0f;
    float ic12V_temperature = 0.0f;
    bool  ic12V_C0_state = false;
    bool  ic12V_C1_state = false;
    bool lamp_reset_flag = false;                       // flag per il reset delle lampade
    bool lampSX_Ready = false;                          // flag che indica se la lampada SX è pronta allo scatto
    bool lampDX_Ready = false;                          // flag che indica se la lampada DX è pronta allo scatto
    char ID_lampSX[ID_NUM_SIZE] = {0};                  // ID della lampada connessa a SX
    char ID_lampDX[ID_NUM_SIZE] = {0};                  // ID della lampada connessa a DX
    bool WarningFlags[WARNING_LAST_INDEX] = {false};    // Array per i flag di warning
    bool ErrorFlags[ERROR_LAST_INDEX] = {false};        // Array per i flag di errori

    // da non stampare
    unsigned long last_log_time = 0;                    // Tempo dell'ultimo log effettuato
    unsigned long ota_progress_millis = 0;
    unsigned long last_error_print_time = 0;            // Tempo dell'ultima stampa degli errori
    unsigned long last_lamp_check_time = 0;             // Tempo dell'ultimo controllo delle lampade
    bool id_print_flag = false;                         // flag per la stampa dell'ID
    bool warning_detected = false;                      // flag che indica se è presente un warning
    bool error_detected = false;                        // flag che indica se è presente un errore critico
    bool isLampID_set = false;                          // flag che è true se sono stati letti gli ID dalle lampade
};

extern systemStatusStruct systemStatus;

/*--------------FUNZIONI-------------*/

/**
* FUNZIONE PER INIZIALIZZARE I PIN CONTENUTI IN UN ARRAY
* @param uint8_t array contenete i pin da inizializzare
* @param uint8_t dimensione dell'array [sizeof(array_da_inizializzare)]
* @param byte tipo del pin (INPUT, OUTPUT)
*/
void declaration_function(const uint8_t array[], uint8_t size, byte type);

/**
* FUNZIONE PER IMPOSTARE IL VALORE DEI PIN CONTENUTI IN UN ARRAY
* @param uint8_t array contenete i pin da impostare
* @param uint8_t dimensione dell'array [sizeof(array_da_impostare)]
* @param byte valore del pin (HIGH, LOW)
*/
void set_pin_function(const uint8_t array[], uint8_t size, byte value);

/**
* FUNZIONE PER L'INIZIALIZZAZIONE DI TUTTE LE ISTANZE
*/
void initialize();

/**
* FUNZIONE PER IL SETUP DELLA EEPROM (LETTURA SCRITTURA VARIABILI DI SISTEMA INIZIALI)
*/
void EEPROM_Setup();

/**
* FUNZIONE CHE LOGGA GLI ERRORI CRITICI SOLO UNA VOLTA
* @param uint8_t codice dell'errore
*/
void processError(uint8_t ErrorCode);

/**
* FUNZIONI PER RILEVARE I WARNING E GLI ERRORI DEL SISTEMA
*/
void systemStatusCheck();

/**
* FUNZIONE CHE STAMPA LO STATUS DI TUTTI I WARNING E GLI ERRORI
*/
void printSystemStatus();

/**
* FUNZIONE PER LOGGARE I DATI DI SISTEMA IN UN FORMATO CSV
*/
void logSystemData();

/**
* FUNZIONE PER CONVERTIRE VELOCEMENTE LA LETTURA ANALOGICA DI UN PIN DALL'ADC [V]
* @return float valore di tensione letto dall'ADC, convertito in funzione del partitore impostato
* @param uint8_t numero del pin analogico da leggere
*/
float getAnalogueVoltage(uint8_t pin_number);

/**
* FUNZIONE CHE STAMPA TUTTE LE TENSIONI LETTE DALL'ADC
*/
void print_ADC();

/**
* FUNZIONE CHE CHIEDE L'ID ALLE LAMPADE CONNESSE E LO METTE IN MEMORIA
*/
bool readLampID();

/**
* FUNZIONE PER SCANSIONARE TUTTI I DISPOSITIVI SUL BUS I2C
*/
void scanI2C();

/**
* FUNZIONE CHE CONTROLLA SE LE LAMPADE SONO PRONTE ALLO SCATTO
*/
void FlashReadyCheck();

/**
* FUNZIONE CHE INOLTRA IL COMANDO DI RESET ALLE LAMPADE TRAMITE 485
*/
void resetLamp();

/**
* FUNZIONE CHE LAMPEGGIA IL LED DI DEBUG
* @param uint8_t pin il pin del led di debug da far lampeggiare
*/
void blinkDebugLED(uint8_t pin);

/**
* FUNZIONE CHE RESTITUISCE LA PAGINA HTML PER L'INTERFACCIA WEB
* @return String la pagina HTML
*/
String getHTMLpage();

/**
* FUNZIONE CHE VERIFICA SE L'ID PASSATO SIA VALIDO
* @param char l'ID che deve essere verificato
* @return bool true se è valido
*/
bool checkID(char ID_to_check[ID_NUM_SIZE]);

/*---------------TELNET-------------*/

bool isConnected();

bool connectToWiFi(const char* ssid, const char* password, int max_tries, int pause);

void errorMsg(String error, bool restart);

// (optional) callback functions for telnet events
void onTelnetConnect(String ip);

void onTelnetDisconnect(String ip);

/*
void onTelnetReconnect(String ip);

void onTelnetConnectionAttempt(String ip);

void onTelnetInput(String str);
*/

void setupTelnet();

/*
* FUNZIONE CON IL LOOP DI TELNET E VERIFICA SE CI SONO VALORI DA LEGGERE
*/
String loopTelnet();

/* 
* FUNZIONE CHE SCRIVE SU TELNET UNA STRINGA (VA A CAPO)
* @param String testo da stampare
*/
void writeTelnet(String text);

/**
 * FUNZIONE CHE STAMPA SU TELNET I BUS 485 SE TROVA DEI DATI DA LEGGERE
 */
void print485onTelnet();

#endif