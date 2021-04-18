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
        SoftwareSerial sensorSerial(2, 3); // rx, tx -> yellow, white
        Stream *commSerial = NULL;

        BMA(){
            commSerial = &sensorSerial
            sensorSerial.begin(57600); // begin communication on pins specified
        }

        bool sendPacket(uint8_t pid, uint8_t cmd, uint8_t* data, uint16_t data_length, bool print_data = false){
            // seperate header and address into bytes
            uint8_t header_bytes[2];
            uint8_t address_bytes[4];

            header_bytes[0] = HEADER & 0xFF; // low byte
            header_bytes[1] = (HEADER >> 8) & 0xFF; // high byte
            
            address_bytes[0] = M_ADDRESS & 0xFF; // low byte
            address_bytes[1] = (M_ADDRESS >> 8) & 0xFF;
            address_bytes[2] = (M_ADDRESS >> 16) & 0xFF;
            address_bytes[3] = (M_ADDRESS >> 24) & 0xFF; // high byte
            
            if(data == NULL){
                data_length = 0;
            }
            
            uint16_t packet_length = data_length + 3; // 1 byte command, 2 bytes checksum
            uint16_t check_sum = 0;
            uint8_t packet_length_in_bytes[2];
            uint8_t check_sum_in_bytes[2];
            
            packet_length_in_bytes[0] = packet_length & 0xFF; // get low byte
            packet_length_in_bytes[1] = (packet_length >> 8) & 0xFF; // get high byte

            check_sum = pid + packet_length_in_bytes[0] + packet_length_in_bytes[1] + cmd;

            for (int i = 0; i < data_length; i++){
                check_sum += data[i];
            }

            check_sum_in_bytes[0] = check_sum & 0xFF; // get low byte
            check_sum_in_bytes[1] = (check_sum >> 8) & 0xFF; // get high byte

            if(print_data){
                Serial.println("writing");
            }
            
            // write to serial. high bytes will be transferred first
            // header
            commSerial->write(header_bytes[1]);
            commSerial->write(header_bytes[0]);

            if(print_data){
                Serial.print(header_bytes[1], HEX);
                Serial.println(header_bytes[0], HEX);
            }
            
            // address
            commSerial->write(address_bytes[3]);
            commSerial->write(address_bytes[2]);
            commSerial->write(address_bytes[1]);
            commSerial->write(address_bytes[0]);

            if(print_data){
                Serial.print(address_bytes[3], HEX);
                Serial.print(address_bytes[2], HEX);
                Serial.print(address_bytes[1], HEX);
                Serial.println(address_bytes[0], HEX);
            }
            
            // packet id
            commSerial->write(pid);

            if(print_data){
                Serial.println(pid, HEX);
            }
            
            // packet length
            commSerial->write(packet_length_in_bytes[1]);
            commSerial->write(packet_length_in_bytes[0]);

            if(print_data){
                Serial.print(packet_length_in_bytes[1], HEX);
                Serial.println(packet_length_in_bytes[0], HEX);
            }
            
            // command
            commSerial->write(cmd);

            if(print_data){
                Serial.println(cmd, HEX);
            }
            
            // data
            for(int i = (data_length - 1); i >= 0; i--){
                commSerial->write(data[i]);
                
                if(print_data){
                Serial.print(data[i], HEX);
                }
            }
            if(print_data){
                Serial.println();
            }
            
            // checksum
            commSerial->write(check_sum_in_bytes[1]);
            commSerial->write(check_sum_in_bytes[0]);

            if(print_data){
                Serial.print(check_sum_in_bytes[1], HEX);
                Serial.println(check_sum_in_bytes[0], HEX);
                
                Serial.println("written");
            }
            return true;
        }
};

#endif