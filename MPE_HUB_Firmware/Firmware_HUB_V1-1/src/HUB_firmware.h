#ifndef HUB_FIRMWARE_H
#define HUB_FIRMWARE_H

/*--ASSEGNAZIONE DEI PIN PER VERSIONE 1.0 DELL'HUB--*/

#include <Arduino.h>
#include <Wire.h>
#include <SimpleCLI.h>          // Include per includere interfaccia a linea di comando
#include <ArduinoJson.h>        // Include per la creazione di json
#include <SPI.h>
#include <Adafruit_ADS1X15.h>   // Include per l'ADC I2C
#include <EEPROM.h>                     // Include library to read and write from flash memory

//#define DEBUG
//#define ECO485

#define SERIAL_SPEED    115200  // baud rate seriale
#define SDA_PIN             34  // Pin SDA per I2C
#define SCL_PIN             48  // Pin SCL per I2C

#define EEPROM_SIZE         0x20    // Il numero di bit da utilizzare della EEPROM
#define ADDR_ID_NUM         0x10    // Indirizzo iniziale di scrittura ID_NUM e scrive i successivi ID_NUM_SIZE byte
#define ID_NUM_SIZE         8       // Numero di byte dell'ID (8 byte)

#define RST_GPIO            5   // Pin di reset verso W5500
#define INT_GPIO            6   // Pin di interrupt del W5500
#define MISO_GPIO           8   // Pin MISO SPI
#define MOSI_GPIO           7   // Pin MOSI SPI
#define SCK_GPIO            9   // Pin SCK SPI
#define CS_GPIO             10  // Pin CS SPI

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
#define R1_ADC_DIVIDER      15  // Valore della prima resistenza del partitore per la lettura da ADC
#define R2_ADC_DIVIDER      30  // Valore della seconda resistenza del partitore (su cui avviene la lettura)

#define BLINK_DELAY_TIME    10  // Tempo di lampeggio del led di debug in ms
#define LAMP_COMMUNICATION_TIMEOUT  6500    // Tempo massimo di attesa per la comunicazione con le lampade in ms

// Array per la dichiarazione degli output (tutti questi controlli sono attivi alti)
const uint8_t OUTPUT_ARRAY[]={RW_485_LED_SX, RW_485_LED_DX, RW_485_ROV,
                              LED_DEBUG_RED, LED_DEBUG_GREEN, BUZZER_DEBUG};

// Array per la dichiarazione degli input
const uint8_t INPUT_ARRAY[]={};

// Dichiarazione porte seriali per 485 dedicati
extern HardwareSerial SerialROV;
extern HardwareSerial SerialLampSX;
extern HardwareSerial SerialLampDX;

/* ----- MAIN PAGE WEB SERVER ------ */
// HTML welcome page with JS
const char MAIN_page[12000] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>HUB MPE</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>

        body {
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        display: flex;
        justify-content: center;
        align-items: center;
        margin: 0;
        padding: 20px;
        min-height: 100vh;
        background: linear-gradient(135deg, #fdfdfd, #f0f4f9);
        color: #333;
        text-align: center;
        }

        .page {
        width: 100%;
        max-width: 1200px;
        display: flex;
        flex-direction: column;
        align-items: center;
        }

        .page-title {
        text-align: center;
        margin-bottom: 40px;
        font-size: 2.5rem;
        font-weight: bold;
        letter-spacing: 1px;
        color: #0078d7;
        text-shadow: 0 2px 5px rgba(0, 120, 215, 0.15);
        }

        .gridcontainer {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
        gap: 30px;
        width: 100%;
        align-items: center;
        justify-items: center;
        }

        .container {
        display: flex;
        flex-direction: column;
        justify-content: center;
        gap: 20px;
        }

        .card {
        display: flex;
        flex-direction: column;
        background: #ffffff;
        border: 1px solid #e5e9f0;
        padding: 25px;
        margin: 15px;
        border-radius: 15px;
        box-shadow: 0 4px 12px rgba(0, 0, 0, 0.05);
        width: 320px;
        text-align: center;
        transition: transform 0.3s ease, box-shadow 0.3s ease;
        }

        .card:hover {
        transform: translateY(-6px);
        box-shadow: 0 10px 25px rgba(0, 0, 0, 0.12);
        }

        h2 {
        color: #222;
        margin-bottom: 20px;
        }

        .data {
        font-size: 1.1em;
        margin: 10px 0;
        font-weight: 500;
        }

        .data i {
        margin-right: 8px;
        color: #0078d7;
        }

        .button-row {
        display: flex;
        flex-direction: column;
        justify-content: center;
        gap: 12px;
        width: 100%;
        }

        button {
        padding: 12px 40px;
        font-size: 16px;
        font-weight: bold;
        border: none;
        border-radius: 10px;
        cursor: pointer;
        background: linear-gradient(135deg, #0078d7, #00a6fb);
        color: white;
        transition: 0.3s;
        width: 100%;
        box-shadow: 0 4px 10px rgba(0, 150, 255, 0.25);
        }

        button i {
        margin-right: 8px;
        }

        button:hover {
        background: linear-gradient(135deg, #0056a1, #0084c7);
        box-shadow: 0 6px 15px rgba(0, 100, 200, 0.35);
        transform: scale(1.05);
        }

        .switch-container {
        font-size: 18px;
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 25px;
        padding: 10px 5px;
        }

        .switch-label {
        font-weight: bold;
        letter-spacing: 1px;
        color: #0078d7;
        }

        .switch {
        position: relative;
        display: inline-block;
        width: 60px;
        height: 32px;
        }

        .switch input { 
        opacity: 0;
        width: 0;
        height: 0;
        }

        .slider {
        position: absolute;
        cursor: pointer;
        top: 0; left: 0;
        right: 0; bottom: 0;
        background-color: #ccc;
        transition: .4s;
        border-radius: 34px;
        }

        .slider:before {
        position: absolute;
        content: "";
        height: 24px; width: 24px;
        left: 4px; bottom: 4px;
        background: #fff;
        transition: .4s;
        border-radius: 50%;
        box-shadow: 0 2px 5px rgba(0,0,0,0.2);
        }

        input:checked + .slider {
        background: linear-gradient(135deg, #00c853, #00e676);
        }

        input:checked + .slider:before {
        transform: translateX(26px);
        }

  </style>

  <script>
  
    function fetchData() {
        fetch("/sensor")
            .then(response => response.json())
            .then(data => {

            document.getElementById("Current_Flash_1").innerHTML = data.Current_Flash_1.toFixed(2) + " A";
            document.getElementById("Current_Flash_2").innerHTML = data.Current_Flash_2.toFixed(2) + " A";
            document.getElementById("Voltage_24V").innerHTML = data.Voltage_24V.toFixed(2) + " V";
            document.getElementById("Chip_Temp_24V").innerHTML = data.Chip_Temp_24V.toFixed(2) + " C";
            document.getElementById("Current_BD3D").innerHTML = data.Current_BD3D.toFixed(2) + " A";
            document.getElementById("Current_IPCam").innerHTML = data.Current_IPCam.toFixed(2) + " A";
            document.getElementById("Voltage_12V").innerHTML = data.Voltage_12V.toFixed(2) + " V";
            document.getElementById("Chip_Temp_12V").innerHTML = data.Chip_Temp_12V.toFixed(2) + " C";

            refreshSwitch("12V_0", data["12V_0_State"]);
            refreshSwitch("12V_1", data["12V_1_State"]);
            refreshSwitch("24V_0", data["24V_0_State"]);
            refreshSwitch("24V_1", data["24V_1_State"]);
            })
            .catch(err => console.error("Errore nel fetch:", err));
    }

    function refreshSwitch(device, state) {
        const checkbox = document.querySelector(`input[type="checkbox"][data-id="${device}"]`);
        const label = document.querySelector(`.switch-label[data-id="${device}"]`);
        if (!checkbox || !label) return;

        checkbox.checked = state;
        label.textContent = `${getDeviceLabel(device)} ${state ? 'ON' : 'OFF'}`;
    }

    function toggleState(el) {
        const device = el.dataset.id;
        const state = el.checked;
        refreshSwitch(device, state);
        sendState(getDeviceURLComponent(device), state ? 'on' : 'off');
    }

    function sendState(device, state) {
        const url = `/${device}?state=${state}`;
        console.log(`Invio richiesta a: ${url}`);
        fetch(url)
            .then(res => res.text())
            .then(data => console.log("Risposta dal server:", data))
            .catch(err => console.error("Errore:", err));
    }

    function sendReset() {
        fetch("/lamp/reset");
    }

    function torchON() {
        fetch("/lamp/torch?power=1")
            .then(res => res.text())
            .then(data => console.log("Risposta dal server:", data))
            .catch(err => console.error("Errore:", err));
    }

    function torchOFF() {
        fetch("/lamp/torch?power=0")
            .then(res => res.text())
            .then(data => console.log("Risposta dal server:", data))
            .catch(err => console.error("Errore:", err));
    }

    function getDeviceLabel(device) {
        switch (device) {
            case "12V_0": return "IPCam";
            case "12V_1": return "BlueDepth";
            case "24V_0": return "Lamp SX";
            case "24V_1": return "Lamp DX";
            default: return device;
        }
    }

    function getDeviceURLComponent(device) {
        switch (device) {
            case "12V_0": return "IPCam";
            case "12V_1": return "BD3D";
            case "24V_0": return "lampSX";
            case "24V_1": return "lampDX";
            default: return device;
        }
    }

    document.addEventListener("DOMContentLoaded", function() {
        fetchData();
    });

    setInterval(fetchData, 2000);
  </script>

</head>
<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.0/css/all.min.css">
<body>
    <div class="page">
        <h2 class="page-title">HUB MPE</h2>
    
        <div class="gridcontainer">
        
            <div class="container">
        
                <div class="card">
                    <div class="data"><i class="fas fa-bolt"></i> Current Flash 1: <span id="Current_Flash_1">--</span></div>
                    <div class="data"><i class="fas fa-bolt"></i> Current Flash 2: <span id="Current_Flash_2">--</span></div>
                    <div class="data"><i class="fas fa-plug"></i> Voltage 24V: <span id="Voltage_24V">--</span></div>
                    <div class="data"><i class="fas fa-thermometer-half"></i> Chip Temp 24V: <span id="Chip_Temp_24V">--</span></div>
                    <div class="data"><i class="fas fa-camera"></i> Current BD3D: <span id="Current_BD3D">--</span></div>
                    <div class="data"><i class="fas fa-video"></i> Current IPCam: <span id="Current_IPCam">--</span></div>
                    <div class="data"><i class="fas fa-plug"></i> Voltage 12V: <span id="Voltage_12V">--</span></div>
                    <div class="data"><i class="fas fa-thermometer-half"></i> Chip Temp 12V: <span id="Chip_Temp_12V">--</span></div>
                </div>
            </div>
            
            <div class="container">
                <div class="card">
                    <div class="switch-container">
                        <span class="switch-label" data-id="12V_0">IPCam OFF</span>
                        <label class="switch">
                        <input type="checkbox" data-id="12V_0" onchange="toggleState(this)">
                        <span class="slider"></span>
                        </label>
                    </div>
                    <div class="switch-container">
                        <span class="switch-label" data-id="12V_1">BlueDepth OFF</span>
                        <label class="switch">
                        <input type="checkbox" data-id="12V_1" onchange="toggleState(this)">
                        <span class="slider"></span>
                        </label>
                    </div>
                    <div class="switch-container">
                        <span class="switch-label" data-id="24V_0">LampSX OFF</span>
                        <label class="switch">
                        <input type="checkbox" data-id="24V_0" onchange="toggleState(this)">
                        <span class="slider"></span>
                        </label>
                    </div>
                    <div class="switch-container">
                        <span class="switch-label" data-id="24V_1">LampDX OFF</span>
                        <label class="switch">
                        <input type="checkbox" data-id="24V_1" onchange="toggleState(this)">
                        <span class="slider"></span>
                        </label>
                    </div>
                </div>

                <div class="card">
                    <div class="button-row">
                        <button onclick="torchON()"><i class="fas fa-sun"></i> TORCH ON</button>
                        <button onclick="torchOFF()"><i class="fas fa-moon"></i> TORCH OFF</button>
                        <button onclick="sendReset()"><i class="fas fa-undo"></i> RESET</button>
                    </div>
                </div>
            </div>
        </div>
    </div>
</body>
</html>
)rawliteral";

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
*
*/
void initialize();

/* 
* FUNZIONE PER CONVERTIRE VELOCEMENTE LA LETTURA ANALOGICA DI UN PIN DALL'ADC [V]
* @return float valore di tensione letto dall'ADC, convertito in funzione del partitore impostato
* @param uint8_t numero del pin analogico da leggere
*/
float getAnalogueVoltage(uint8_t pin_number);

/*
* FUNZIONE CHE STAMPA TUTTE LE TENSIONI LETTE DALL'ADC
*/
void print_ADC();

/* 
* FUNZIONE CHE STAMPA IL TESTO SUL 485 DEDICATO AL ROV (bisogna esplicitare '\n e \r')
* @param String il testo da stampare
*/
void write485ROV(String text);

/* 
* FUNZIONE CHE STAMPA IL TESTO SUL 485 DEDICATO ALLA LAMPADA SX (bisogna esplicitare '\n e \r')
* @param String il testo da stampare
*/
void write485LampSX(String text);

/*
* FUNZIONE CHE STAMPA IL TESTO SUL 485 DEDICATO ALLA LAMPADA DX (bisogna esplicitare '\n e \r')
* @param String il testo da stampare
*/
void write485LampDX(String text);

/* 
* FUNZIONE PER SCANSIONARE TUTTI I DISPOSITIVI SUL BUS I2C
*/
void scanI2C();

/*
* FUNZIONE CHE VERIFICA SE UN CARATTERE è UNA LETTERA
* @param char c il carattere da verificare
* @return bool restituisce TRUE se il carattere è una lettera, FALSE altrimenti
*/
bool isLetter(char c);

/*
* FUNZIONE CHE INOLTRA IL COMANDO DI RESET ALLE LAMPADE TRAMITE 485
*/
void resetLamp();

/*
* FUNZIONE CHE LAMPEGGIA IL LED DI DEBUG
* @param uint8_t pin il pin del led di debug da far lampeggiare
*/
void blinkDebugLED(uint8_t pin);

/*---------------TELNET-------------*/

bool isConnected();

bool connectToWiFi(const char* ssid, const char* password, int max_tries, int pause);

void errorMsg(String error, bool restart);

// (optional) callback functions for telnet events
void onTelnetConnect(String ip);

void onTelnetDisconnect(String ip);

void onTelnetReconnect(String ip);

void onTelnetConnectionAttempt(String ip);

void onTelnetInput(String str);

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

#endif