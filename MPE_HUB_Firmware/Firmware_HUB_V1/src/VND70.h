#ifndef VND70_H
#define VND70_H

#include <Arduino.h>

// Struttura che associa un ID ai suoi pin per VND70
typedef struct {
    uint8_t ID;
    uint8_t MultiSense;
    uint8_t EnableChannel0;
    uint8_t EnableChannel1;
    uint8_t SensingEnable;      // [Active HIGH]
    uint8_t SEL_0;              // Sense MUX 0 [Active HIGH]
    uint8_t SEL_1;              // Sense MUX 1 [Active HIGH]
} VND70Pins;

class VND70 {
    public:
        // Registra un nuovo componente VND70 (massimo MAX_COMPONENTS)
        static bool registerComponent(uint8_t ID, uint8_t MultiSense, uint8_t EnableChannel0,
            uint8_t EnableChannel1, uint8_t SensingEnable, uint8_t SEL_0, uint8_t SEL_1);

        // Inizializza i pin di tutti i componenti registrati
        static void begin();

        // Spegne le uscite dell'istenza "ID"
        static void standby(uint8_t ID);

        // Azione sul canale 0 del componente "ID" (true -> ON; false -> OFF)
        static void channel_0(uint8_t ID, bool channel_state);

        // Azione sul canale 1 del componente "ID" (true -> ON; false -> OFF)
        static void channel_1(uint8_t ID, bool channel_state);

        // Lettura di tensione del componente "ID"
        static int16_t readVoltage(uint8_t ID);

        // Lettura di corrente del componente "ID" sul canale "channel"
        static int16_t readCurrent(uint8_t ID, uint8_t channel);

        // Lettura di temperatura del componente "ID"
        static int16_t readTemperature(uint8_t ID);

    private:
        static const uint8_t MAX_COMPONENTS = 10;
        static VND70Pins components[];
        static uint8_t count;

        // Trova l’indice in components[] dato un ID
        static int8_t findIndex(uint8_t ID);
};

#endif