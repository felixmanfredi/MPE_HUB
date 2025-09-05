#include "VND70.h"
#include "HUB_firmware.h"

VND70Pins VND70::components[VND70::MAX_COMPONENTS];
uint8_t   VND70::count = 0;

bool VND70::registerComponent(uint8_t ID, uint8_t MultiSense, uint8_t EnableChannel0,
    uint8_t EnableChannel1, uint8_t SEL_0, uint8_t SEL_1) {
    if (count >= MAX_COMPONENTS) return false;          // array pieno
    if (findIndex(ID) >= 0)     return false;           // ID già registrato

    components[count++] = { ID, MultiSense, EnableChannel0, EnableChannel1, SEL_0, SEL_1};
    return true;
}

void VND70::begin() {
    for (uint8_t i = 0; i < count; i++) {
        pinMode(components[i].MultiSense, INPUT);
        pinMode(components[i].EnableChannel0, OUTPUT);
        pinMode(components[i].EnableChannel1, OUTPUT);
        pinMode(components[i].SEL_0, OUTPUT);
        pinMode(components[i].SEL_1, OUTPUT);
        digitalWrite(components[i].EnableChannel0, LOW);
        digitalWrite(components[i].EnableChannel1, LOW);
        digitalWrite(components[i].SEL_0, LOW);
        digitalWrite(components[i].SEL_1, LOW);
        components[i].channel_0_state = false;
        components[i].channel_1_state = false;
    }
}

void VND70::ALLon(uint8_t ID) {
    int8_t idx = findIndex(ID);
    if (idx < 0) return;
    digitalWrite(components[idx].EnableChannel0, HIGH);
    digitalWrite(components[idx].EnableChannel1, HIGH);
    components[idx].channel_0_state = true;
    components[idx].channel_1_state = true;
}

void VND70::standby(uint8_t ID) {
    int8_t idx = findIndex(ID);
    if (idx < 0) return;
    digitalWrite(components[idx].EnableChannel0, LOW);
    digitalWrite(components[idx].EnableChannel1, LOW);
    digitalWrite(components[idx].SEL_0, LOW);
    digitalWrite(components[idx].SEL_1, LOW);
    components[idx].channel_0_state = false;
    components[idx].channel_1_state = false;
}

void VND70::channel_0(uint8_t ID, bool channel_state) {
    int8_t idx = findIndex(ID);
    if (idx < 0) return;
    if (channel_state){
        digitalWrite(components[idx].EnableChannel0, HIGH);
        components[idx].channel_0_state = true;
    } else {
        digitalWrite(components[idx].EnableChannel0, LOW);
        components[idx].channel_0_state = false;
    }
}

void VND70::channel_1(uint8_t ID, bool channel_state) {
    int8_t idx = findIndex(ID);
    if (idx < 0) return;
    if (channel_state){
        digitalWrite(components[idx].EnableChannel1, HIGH);
        components[idx].channel_1_state = true;
    } else {
        digitalWrite(components[idx].EnableChannel1, LOW);
        components[idx].channel_1_state = false;
    }
}

float VND70::readVoltage(uint8_t ID){
    int8_t idx = findIndex(ID);
    digitalWrite(components[idx].SEL_0, HIGH);           // 11 per leggere la tensione sul chip
    digitalWrite(components[idx].SEL_1, HIGH);
    delay(5);
    float temp_reading = getAnalogueVoltage(components[idx].MultiSense);
    return temp_reading*8;      // da datsheet Vsense = Vcc/8
}

float VND70::readCurrent(uint8_t ID, uint8_t channel){
    int8_t idx = findIndex(ID);
    if (channel == 0){
        digitalWrite(components[idx].SEL_0, LOW);      // 00 per leggere la corrente sul primo canale
        digitalWrite(components[idx].SEL_1, LOW);
    } else if (channel == 1){
        digitalWrite(components[idx].SEL_0, LOW);      // 01 per leggere la corrente sul secondo canale
        digitalWrite(components[idx].SEL_1, HIGH);
    } else {
        writeTelnet("Lettura di corrente fallita, canale è diverso da 0 o 1");
        return -1;
    }
    delay(5);
    float temp_reading = getAnalogueVoltage(components[idx].MultiSense);
    return temp_reading;
}

float VND70::readTemperature(uint8_t ID){
    int8_t idx = findIndex(ID);
    digitalWrite(components[idx].SEL_0, HIGH);           // 10 per leggere la tensione sul chip
    digitalWrite(components[idx].SEL_1, LOW);
    delay(5);
    float temp_reading = getAnalogueVoltage(components[idx].MultiSense);
    return temp_reading;
}

bool VND70::channel_0_state(uint8_t ID) {
    int8_t idx = findIndex(ID);
    if (idx < 0) return false;
    return components[idx].channel_0_state;
}

bool VND70::channel_1_state(uint8_t ID) {
    int8_t idx = findIndex(ID);
    if (idx < 0) return false;
    return components[idx].channel_1_state;
}

int8_t VND70::findIndex(uint8_t ID) {
    for (uint8_t i = 0; i < count; i++) {
        if (components[i].ID == ID) return i;
    }
    return -1;
}