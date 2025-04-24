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

        // Azione 1 sul componente (es. attiva pinA)
        static void enable_channel_0(uint8_t id);

        // Azione 2 sul componente (es. attiva pinB)
        static void enable_channel_1(uint8_t id);

    private:
        static const uint8_t MAX_COMPONENTS = 10;
        static VND70Pins components[];
        static uint8_t count;

        // Trova l’indice in components[] dato un ID
        static int8_t findIndex(uint8_t ID);
};

#endif