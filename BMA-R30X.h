/*
  BMA-R30X.h - Library for iterfacing with r30x series fingerprint scanners.
  Created by Batman - CEO Meem Ent., April 18, 2021.
*/

#ifndef BMA-R30X_h
#define BMA-R30X_h

#include "Arduino.h"

#include <SoftwareSerial.h>

// default module password and address. each 4 bytes
#define M_PASSWORD 0x00000000
#define M_ADDRESS 0xFFFFFFFF

// packet IDs.
#define HEADER 0xEF01
#define PID_COMMAND 0x01
#define PID_DATA 0x02
#define PID_ACKNOWLEDGE 0x07
#define PID_PACKET_END 0x08
#define PID_DATA_END 0x08

// Module command codes
#define CMD_VERIFY_PASSWORD 0x13
#define CMD_COLLECT_FINGER_IMAGE 0x01
#define CMD_GEN_CHAR_FILE 0x02
#define CMD_REG_MODEL 0x05
#define CMD_STORE_TEMPLATE 0x06
#define CMD_SEARCH_LIBRARY 0x04

// length of buffer used to read serial data
#define FPS_DEFAULT_SERIAL_BUFFER_LENGTH 300

// default timeout to wait for response from scanner
#define DEFAULT_TIMEOUT 2000

class BMA{
    public:
        uint8_t *rx_data = NULL;
        uint16_t rx_data_length = 0;
        Stream *commSerial = NULL;

        BMA();
        bool sendPacket(uint8_t pid, uint8_t cmd, uint8_t* data, uint16_t data_length, bool print_data = false);
        uint8_t receivePacket(uint32_t timeout = DEFAULT_TIMEOUT, bool print_data = false);
        bool verifyPassword(uint32_t password = M_PASSWORD);
        bool enrollFinger();
        bool fingerSearch();

    private:
        SoftwareSerial sensorSerial(2, 3); // rx, tx -> yellow, white

        uint8_t collectFingerImage(uint8_t rx_response);
        uint8_t generateCharacterFile(uint8_t bufferId[]);
};

#endif