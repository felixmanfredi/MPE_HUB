#include "VND70.h"

VND70Pins VND70::components[VND70::MAX_COMPONENTS];
uint8_t   VND70::count = 0;

bool VND70::registerComponent(uint8_t ID, uint8_t MultiSense, uint8_t EnableChannel0,
    uint8_t EnableChannel1, uint8_t SensingEnable, uint8_t SEL_0, uint8_t SEL_1) {
    if (count >= MAX_COMPONENTS) return false;          // array pieno
    if (findIndex(ID) >= 0)     return false;           // ID già registrato

    components[count++] = { ID, MultiSense, EnableChannel0, EnableChannel1, SensingEnable, SEL_0, SEL_1};
    return true;
}

void VND70::begin() {
    for (uint8_t i = 0; i < count; i++) {
        pinMode(components[i].MultiSense, INPUT);
        pinMode(components[i].EnableChannel0, OUTPUT);
        pinMode(components[i].EnableChannel1, OUTPUT);
        pinMode(components[i].SensingEnable, OUTPUT);
        pinMode(components[i].SEL_0, OUTPUT);
        pinMode(components[i].SEL_1, OUTPUT);
        digitalWrite(components[i].EnableChannel0, HIGH);   // C'è un MOSFET intermedio che inverte la logica
        digitalWrite(components[i].EnableChannel1, HIGH);
        digitalWrite(components[i].SensingEnable, HIGH);
        digitalWrite(components[i].SEL_0, HIGH);
        digitalWrite(components[i].SEL_1, HIGH);
    }
}

void VND70::enable_channel_0(uint8_t ID) {
    int8_t idx = findIndex(ID);
    if (idx < 0) return;
    digitalWrite(components[idx].EnableChannel0, LOW);
    /*digitalWrite(components[idx].EnableChannel1, HIGH);
    digitalWrite(components[idx].SensingEnable, HIGH);
    digitalWrite(components[idx].SEL_0, HIGH);
    digitalWrite(components[idx].SEL_1, HIGH);*/
}

void VND70::enable_channel_1(uint8_t ID) {
    int8_t idx = findIndex(ID);
    if (idx < 0) return;
    digitalWrite(components[idx].EnableChannel1, LOW);
    /*digitalWrite(components[idx].EnableChannel0, HIGH);
    digitalWrite(components[idx].SensingEnable, HIGH);
    digitalWrite(components[idx].SEL_0, HIGH);
    digitalWrite(components[idx].SEL_1, HIGH);*/
}

int8_t VND70::findIndex(uint8_t ID) {
    for (uint8_t i = 0; i < count; i++) {
        if (components[i].ID == ID) return i;
    }
    return -1;
}