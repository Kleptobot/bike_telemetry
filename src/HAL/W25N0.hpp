#ifndef _W25N0_H_
#define _W25N0_H_

#pragma once
#include <SPI.h>

constexpr uint8_t FLASH_CS = D7;

class WN250  {
public:
    void selectFlash()
    {
        digitalWrite(FLASH_CS, LOW);
    }

    void deselectFlash()
    {
        digitalWrite(FLASH_CS, HIGH);
    }

    void resetFlash()
    {
        selectFlash();
        SPI.transfer(0xFF);
        deselectFlash();

        delay(1);
    }

    void readJEDEC(uint8_t& manufacturer,
                uint8_t& device1,
                uint8_t& device2)
    {
        selectFlash();

        SPI.transfer(0x9F);

        SPI.transfer(0x00); // dummy byte

        manufacturer = SPI.transfer(0x00);
        device1      = SPI.transfer(0x00);
        device2      = SPI.transfer(0x00);

        deselectFlash();
    }

    void test()
    {

        pinMode(FLASH_CS, OUTPUT);
        deselectFlash();

        SPI.begin();

        SPI.beginTransaction(
            SPISettings(48000000,
                MSBFIRST,
                SPI_MODE0));

        delay(100);

        Serial.println();
        Serial.println("W25N01 Test");

        resetFlash();

        uint8_t mf;
        uint8_t id1;
        uint8_t id2;

        readJEDEC(mf, id1, id2);

        Serial.print("Manufacturer: 0x");
        Serial.println(mf, HEX);

        Serial.print("Device ID 1: 0x");
        Serial.println(id1, HEX);

        Serial.print("Device ID 2: 0x");
        Serial.println(id2, HEX);
    }
};

#endif // _W25N0_H_