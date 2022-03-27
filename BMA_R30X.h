/*
  BMA-R30X.h - Library for iterfacing with r30x series fingerprint scanners.
  Created by Batman - CEO Meem Ent.
  Created on April 18, 2021.
*/

#ifndef BMA_R30X_h
#define BMA_R30X_h

#include "Arduino.h"
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

// EEPROM data locations.
#define NETWORK_LENGTH 0
#define PASSWORD_LENGTH 1

// OLED screen resolution ('0.96)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// default module password and address.
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
#define CMD_UPLOAD_TEMPLATE 0x08
#define CMD_SEARCH_LIBRARY 0x04
#define CMD_READ_TEMPLATE 0x07

// length of buffer used to read serial data
#define FPS_DEFAULT_SERIAL_BUFFER_LENGTH 300

// default timeout to wait for response from scanner
#define DEFAULT_TIMEOUT 2000

class BMA{
    public:
      uint8_t *rx_data = NULL;
      uint8_t *template_file = NULL;
      uint8_t finger_location = 0;
      uint16_t rx_data_length = 0;
      uint16_t template_length = 0;
      String organizationID;
      Adafruit_SSD1306 *display = NULL;

      BMA();
      bool sendPacket(uint8_t pid, uint8_t cmd, uint8_t* data, uint16_t data_length, bool print_data = false);
      uint8_t receivePacket(uint32_t timeout = DEFAULT_TIMEOUT, bool print_data = false);
      uint8_t receiveTemplate();
      bool verifyPassword(uint32_t password = M_PASSWORD);
      uint16_t enrollFinger();
      bool fingerSearch();
      bool uploadTemplate();
      bool readTemplateFromLib();
      void displayOLED(char* msg);

    private:
      uint8_t collectFingerImage();
      uint8_t generateCharacterFile(uint8_t bufferId[]);
};

struct Attendance{
  uint16_t authID;
  String date;
  String time;
};

#endif