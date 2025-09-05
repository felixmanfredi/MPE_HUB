#ifndef VND70_H
#define VND70_H

#include <Arduino.h>

// Struttura che associa un ID ai suoi pin per VND70
typedef struct {
    uint8_t ID;
    uint8_t MultiSense;
    uint8_t EnableChannel0;
    uint8_t EnableChannel1;
    uint8_t SEL_0;              // Sense MUX 0 [Active HIGH]
    uint8_t SEL_1;              // Sense MUX 1 [Active HIGH]
    bool channel_0_state;    // Stato del canale 0 (true -> acceso / false -> spento)
    bool channel_1_state;    // Stato del canale 1 (true -> acceso / false -> spento)
    bool sensing_state;      // Stato del sensing (true -> acceso / false -> spento)
} VND70Pins;

class VND70 {
    public:
        // Registra un nuovo componente VND70 (massimo MAX_COMPONENTS)
        static bool registerComponent(uint8_t ID, uint8_t MultiSense, uint8_t EnableChannel0,
            uint8_t EnableChannel1, uint8_t SEL_0, uint8_t SEL_1);

        // Inizializza i pin di tutti i componenti registrati
        static void begin();

        // Accende le uscite dell'istenza "ID"
        static void ALLon(uint8_t ID);

        // Spegne le uscite dell'istenza "ID"
        static void standby(uint8_t ID);

        // Azione sul canale 0 del componente "ID" (true -> ON; false -> OFF)
        static void channel_0(uint8_t ID, bool channel_state);

        // Azione sul canale 1 del componente "ID" (true -> ON; false -> OFF)
        static void channel_1(uint8_t ID, bool channel_state);

        // Azione sullo stato di sensing del componente "ID" (true -> ON; false -> OFF)
        static void sensing(uint8_t ID, bool sensing_state);

        // Lettura di tensione del componente "ID"
        static float readVoltage(uint8_t ID);

        // Lettura di corrente del componente "ID" sul canale "channel"
        static float readCurrent(uint8_t ID, uint8_t channel);

        // Lettura di temperatura del componente "ID"
        static float readTemperature(uint8_t ID);

        // Lettura dello stato del canale 0 del componente "ID"
        static bool channel_0_state(uint8_t ID);

        // Lettura dello stato del canale 1 del componente "ID"
        static bool channel_1_state(uint8_t ID);

    private:
        static const uint8_t MAX_COMPONENTS = 10;
        static VND70Pins components[];
        static uint8_t count;

        // Trova l’indice in components[] dato un ID
        static int8_t findIndex(uint8_t ID);
};

#endif