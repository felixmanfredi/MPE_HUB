#include "RS485.h"

RS485Bus::RS485Bus(HardwareSerial &serialPort, uint8_t rw, unsigned long timeout)
    : serial(serialPort), rwPin(rw), defaultTimeout(timeout), busBusy(false) {
    pinMode(rwPin, OUTPUT);
    digitalWrite(rwPin, LOW);  // start in RX
}

// Funzione pubblica: invia e riceve
String RS485Bus::sendMessage(String msg, unsigned long timeout) {
    if (!busEnabled) return ""; // bus disattivato return subito

    if (timeout == 0) timeout = defaultTimeout;

    // Attende il bus libero
    while (busBusy) delay(1);
    busBusy = true;
    flush();

    transmit(msg);
    delay(5);
    String resp = receive(timeout);

    busBusy = false;
    return resp;
}

/**
 * @brief Send a message without waiting for a response
 */
void RS485Bus::sendMessageNoResponse(String msg) {
    if (!busEnabled) return; // bus disattivato return subito

    // wait until bus is free
    while (busBusy) delay(1);
    busBusy = true;

    transmit(msg);

    busBusy = false;
}

/**
 * @brief Clear the RX buffer
 */
void RS485Bus::flush() {
    while (serial.available()) {
        serial.read(); // discard data
    }
}

/**
 * @brief Checks if there are at least `minBytes` available on the RS485 bus.
 */
String RS485Bus::checkMessage(size_t minBytes, unsigned long timeout) {
    if (!busEnabled) return ""; // bus disattivato return subito

    if (timeout == 0) timeout = defaultTimeout;

    if (!busBusy){
        if (serial.available() >= (int)minBytes) {
            return receive(timeout);
        }
    }
    return ""; // nessun messaggio sufficiente
}

// Trasmissione
void RS485Bus::transmit(String msg) {
    digitalWrite(rwPin, HIGH);      // attiva TX
    serial.print(msg);
    serial.flush();                 // aspetta fine trasmissione
    digitalWrite(rwPin, LOW);       // torna in RX
}

// Ricezione con timeout
String RS485Bus::receive(unsigned long timeout) {
    if (timeout == 0) timeout = defaultTimeout;

    unsigned long start = millis();
    String response = "";

    while (millis() - start < timeout) {
        if (serial.available()) {
            char c = (char)serial.read();
            if (c == '\n') {
                break; // terminatore trovato
                response += '\0';
            }
            response += c;
        }
    }

    return response;
}
