#include "log.h"

// Costruttore della classe Logger
Logger::Logger(const char* filename) {
    _filename = filename; // Salva il nome del file di log su cui scrivere/leggere
}

// Inizializza il filesystem LittleFS
bool Logger::begin(bool formatOnFail) {
    if(!LittleFS.begin(formatOnFail)) {          // Prova a montare il filesystem
        //Serial.println("LittleFS mount failed"); // Se fallisce, messaggio di errore
        return false;
    }
    //Serial.println("LittleFS montato correttamente");
    return true;
}

// Scrive una riga di log nel file
void Logger::log(const String &data) {
    File file = LittleFS.open(_filename, FILE_APPEND); // Apre il file in append
    if(!file) {  // Se l'apertura fallisce da errore
        //Serial.println("Errore apertura file per append");
        return;
    }
    lastlogtime = millis();
    file.println(data); // Scrive i dati e va a capo
    file.close();       // Chiude il file
}

// Legge tutto il contenuto del file di log e lo stampa
void Logger::readAll(Stream &out) {
    File file = LittleFS.open(_filename, "r");
    if (!file) return;

    while (file.available()) {
        out.write(file.read());  // scrive su qualsiasi stream
    }

    file.close();
}

// Mostra lo stato del filesystem (spazio totale e usato)
void Logger::checkFS(Stream &out) {
    size_t total = LittleFS.totalBytes();
    size_t used  = LittleFS.usedBytes();
    out.print("\n\rFS Totale: " + String(total) + " bytes, Usato: " + String(used) + " bytes\n\r");

    File root = LittleFS.open("/");
    if (root) {
        File file = root.openNextFile();
        while (file) {
            String fileName = file.name();
            size_t fileSize = file.size();
            out.print("\n\rFile: " + String(fileName) + ", Dimensione: " + String(fileSize) + " bytes\n\r");
            file = root.openNextFile();
        }
        root.close(); //chiudo la lettura dei file
    }
}
