/*
    BMA-R30X.h - Library for interfacing with r30x series fingerprint scanners.
    Created by Batman - CEO Meem Ent., April 18, 2021.
*/

#include "BMA_R30X.h"

BMA::BMA(){
    display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
    if(!display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        // Serial.println(("SSD1306 allocation failed"));
        // cue it with LEDs
    }

    display->setTextSize(1);
    display->setTextColor(WHITE);

    uint8_t ssid_len = 0, password_len = 0;
    EEPROM.begin(512);
    EEPROM.get(NETWORK_LENGTH, ssid_len);
    EEPROM.get(PASSWORD_LENGTH, password_len);

    // ssid and password start at 2. organization id is 5 bytes
    if (ssid_len > 0 && password_len > 0){
        finger_location = 2 + ssid_len + password_len + 5;
        attendance_count = 2 + ssid_len + password_len + 5 + 1;
        attendance_store = 2 + ssid_len + password_len + 5 + 1 + 1;

        char str;
        for (uint8_t i = 2 + ssid_len + password_len, j = 0; j < 5; i++, j++){
            EEPROM.get(i, str);
            organizationID += str;
        }
    }
    else displayOLED("Error at startup");

    EEPROM.end();
}

void BMA::displayOLED(char* msg){
    display->clearDisplay();
    display->setCursor(0, 10);
    display->println(msg);
    display->display();
}

bool BMA::sendPacket(uint8_t pid, uint8_t cmd, uint8_t* data, uint16_t data_length, bool print_data){
    // separate header and address into bytes to respect big endian format
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
        Serial.println("Start printing data");

        Serial.print(header_bytes[1], HEX);
        Serial.println(header_bytes[0], HEX);
        Serial.print(address_bytes[3], HEX);
        Serial.print(address_bytes[2], HEX);
        Serial.print(address_bytes[1], HEX);
        Serial.println(address_bytes[0], HEX);
        Serial.println(pid, HEX);
        Serial.print(packet_length_in_bytes[1], HEX);
        Serial.println(packet_length_in_bytes[0], HEX);
        Serial.println(cmd, HEX);
        for(int i = 0; i < data_length; i++){
            Serial.print(data[i], HEX);
        }
        Serial.println();
        Serial.print(check_sum_in_bytes[1], HEX);
        Serial.println(check_sum_in_bytes[0], HEX);
        
        Serial.println("End");
    }

    // write to serial. high bytes will be transferred first
    // header
    Serial.write(header_bytes[1]);
    Serial.write(header_bytes[0]);

    // address
    Serial.write(address_bytes[3]);
    Serial.write(address_bytes[2]);
    Serial.write(address_bytes[1]);
    Serial.write(address_bytes[0]);

    // packet id
    Serial.write(pid);
    
    // packet length
    Serial.write(packet_length_in_bytes[1]);
    Serial.write(packet_length_in_bytes[0]);

    // command
    Serial.write(cmd);

    // data
    for(int i = 0; i < data_length; i++){
        Serial.write(data[i]);
    }
    
    // checksum
    Serial.write(check_sum_in_bytes[1]);
    Serial.write(check_sum_in_bytes[0]);

    return true;
}

uint8_t BMA::receivePacket(uint32_t timeout, bool print_data){
    uint8_t serial_buffer[FPS_DEFAULT_SERIAL_BUFFER_LENGTH] = {0}; // will store high byte at start of array
    uint16_t serial_buffer_length = 0;
    uint32_t start_time = millis();

    // wait for response for set timeout
    while(serial_buffer_length < FPS_DEFAULT_SERIAL_BUFFER_LENGTH && millis() - start_time < timeout){
        if(Serial.available()){
            serial_buffer[serial_buffer_length] = Serial.read();
            serial_buffer_length++;
        }
    }

    if(serial_buffer_length == 0){
        Serial.println("received no response");
        return 0x02;
    }

    if(serial_buffer_length < 10){
        Serial.println("bad packet nigga");
        return 0x02;
    }

    uint8_t* data_buffer = new uint8_t[64];
    uint16_t token = 0;   // to iterate over switch cases
    uint8_t packet_type;  // received packet type to use in checking checksum
    uint8_t rx_packet_length[2]; // received packet length
    uint16_t rx_packet_length_2b; // single data type for holding received packet length
    uint16_t data_length; // length of data extracted from packet length
    uint8_t confirmation_code = 0xFF;
    uint8_t checksum_bytes[2]; // checksum in bytes
    uint16_t checksum; // whole checksum representation

    //the following loop checks each segment of the data packet for errors
    while(1){
        switch(token){
        // case 0 & 1 check for header
        case 0:
            // high byte
            if(serial_buffer[token] == ((HEADER >> 8) & 0xFF)) break;
            
            return 0x02;
            
        case 1:
            // low byte
            if(serial_buffer[token] == (HEADER & 0xFF)) break;

            return 0x02;
            
        // cases 2 to 5 check for device address. high to low byte
        case 2:
            if(serial_buffer[token] == ((M_ADDRESS >> 24) & 0xFF) ) break;
            
            return 0x02;

        case 3:
            if(serial_buffer[token] == ((M_ADDRESS >> 16) & 0xFF) ) break;
            
            return 0x02;

        case 4:
            if(serial_buffer[token] == ((M_ADDRESS >> 8) & 0xFF) ) break;
            
            return 0x02;

        case 5:
            if(serial_buffer[token] == (M_ADDRESS & 0xFF) ) break;
            
            return 0x02;

        // check for valid packet type
        case 6:
            if((serial_buffer[token] == PID_DATA) || (serial_buffer[token] == PID_ACKNOWLEDGE) || (serial_buffer[token] == PID_DATA_END)) {
                packet_type = serial_buffer[token];
                break;
            }
            
            return 0x02;

        // read packet data length
        case 7:
            if((serial_buffer[token] > 0) || (serial_buffer[token + 1] > 0)){
                rx_packet_length[0] = serial_buffer[token + 1]; // low byte
                rx_packet_length[1] = serial_buffer[token]; // high byte
                rx_packet_length_2b = uint16_t(rx_packet_length[1] << 8) + rx_packet_length[0]; // the full length
                data_length = rx_packet_length_2b - 3; // 2 checksum , 1 confirmation code
                
                token ++;

                break;
            }
            
            return 0x02;

        // case 8 won't be hit as after case 7, token value is 9
        // read confirmation code
        case 9:
            confirmation_code = serial_buffer[token];
            break;

        // read data
        case 10:
            for(int i = 0; i < data_length; i++){
                data_buffer[(data_length - 1) - i] = serial_buffer[token + i]; // low bytes will be written at end of array. Inherently, high bytes appear first
            }
            break;

        // read checksum
        case 11:
            // if there ain't no data
            if(data_length == 0){
                checksum_bytes[0] = serial_buffer[token]; // low byte
                checksum_bytes[1] = serial_buffer[token - 1]; // high byte
                checksum = uint16_t(checksum_bytes[1] << 8) + checksum_bytes[0];

                uint16_t tmp = 0;

                tmp = packet_type + rx_packet_length[0] + rx_packet_length[1] + confirmation_code;

                // if calculated checksum is equal to received checksum. is good to go
                if(tmp == checksum){
                    if(print_data) Serial.println("no data pass");
                    return confirmation_code;
                }
                if(print_data) Serial.println("no data fail");
                return confirmation_code;

                break;
            }

            // if data is present
            else if((serial_buffer[token + (data_length -1)] > 0) || ((serial_buffer[token + 1 + (data_length -1)] > 0))){
                checksum_bytes[0] = serial_buffer[token + 1 + (data_length -1 )]; // low byte
                checksum_bytes[1] = serial_buffer[token + (data_length -1 )]; // high byte
                checksum = uint16_t (checksum_bytes[1] << 8) + checksum_bytes[0];

                uint16_t tmp = 0;

                tmp = packet_type + rx_packet_length[0] + rx_packet_length[1] + confirmation_code;
                rx_data = new uint8_t[data_length];
                rx_data_length = data_length;
                
                for(int i=0; i < data_length; i++) {
                    tmp += data_buffer[i]; //calculate data checksum
                    rx_data[i] = data_buffer[i]; // get data to be used later
                }

                if(tmp == checksum) {
                    if(print_data) Serial.println("data present pass");
                    return confirmation_code;
                }

                if(print_data) Serial.println("data present fail");
                return confirmation_code;

                break;
            }

            // if checksum is 0
            else {
                if(print_data) Serial.println("checksum 0");
                return confirmation_code;
            }

        default:
            Serial.println("default");
            return confirmation_code;
        }

        token ++;
    }
}

bool BMA::verifyPassword(uint32_t password){
    // store password seperately in 4 bytes.
    uint8_t password_bytes[4] = {0};
    password_bytes[0] = password & 0xFF;
    password_bytes[1] = (password >> 8);
    password_bytes[2] = (password >> 16);
    password_bytes[3] = (password >> 24);


    sendPacket(PID_COMMAND, CMD_VERIFY_PASSWORD, password_bytes, 4);
    uint8_t response = receivePacket(4000);

    if(response == 0x00) return true;
    return false;
}

uint16_t BMA::enrollFinger(){
    // place finger twice and generate character/template file each time, combine them and store to a given location
    EEPROM.begin(512);

    uint8_t rx_response = 0x02;

    rx_response = collectFingerImage();
        
    uint8_t bufferId[1] = {1};
    generateCharacterFile(bufferId);

    delay(1000);
    
    rx_response = collectFingerImage();
        
    bufferId[0] = 2;
    generateCharacterFile(bufferId);

    // combine both templates to make 1 template
    sendPacket(PID_COMMAND, CMD_REG_MODEL, NULL, 0 );
    rx_response = receivePacket();

    if(!rx_response == 0x00) return false;

    uint8_t data[3] = {0};
    uint16_t location;
    EEPROM.get(finger_location, location);
    location++;
    
    data[0] = 0x01; // buffer Id 1
    data[1] = location & 0xFF; // low byte
    data[2] = (location >> 8) & 0xFF; // high byte

    sendPacket(PID_COMMAND, CMD_STORE_TEMPLATE, data, 3);
    rx_response = receivePacket();

    if(rx_response == 0x00){
        EEPROM.put(finger_location, location);
        EEPROM.commit();
        EEPROM.end();
        return location;
    }

    EEPROM.end();
    return 0;
}

bool BMA::fingerSearch(){
    /* 
        Place finger, generate a character/template file and search in library. 
        It'll return fingerID (location it was stored to) and score
    */
    uint8_t rx_response = 0x02;
    uint8_t bufferId[1] = {1};

    rx_response = collectFingerImage();
    generateCharacterFile(bufferId);

    uint8_t data[5] = {0};
    uint16_t number = 4; // number of templates to search. must be +1 from the last location saved
    uint16_t start = 0;

    data[0] = 0x01; // buffer Id
    data[1] = start & 0xFF;
    data[2] = (start >> 8) & 0xFF;
    data[3] = number & 0xFF;
    data[4] = (number >> 8) & 0xFF;

    sendPacket(PID_COMMAND, CMD_SEARCH_LIBRARY, data, 5);
    rx_response = receivePacket();

    if(rx_response == 0x00) return true;
    return false;
}

uint8_t BMA::collectFingerImage(){
    uint8_t rx_response = 0x02;

    while(rx_response == 0x02){
        displayOLED("Place finger");
        sendPacket(PID_COMMAND, CMD_COLLECT_FINGER_IMAGE, NULL, 0);
        rx_response = receivePacket();
    }
    
    displayOLED("Remove finger");
    return rx_response;
}

uint8_t BMA::generateCharacterFile(uint8_t bufferId[]){
    sendPacket(PID_COMMAND, CMD_GEN_CHAR_FILE, bufferId, 1);
    uint8_t rx_response = receivePacket();

    return rx_response;
}

bool BMA::uploadTemplate(){
    /*
        Upload template from charBuffer1 to upper computer.
        Currently acknowledge packet is prepended and end-of-data package is appended.
        Will clean it up after research into how to clean data when we reach the phase of downloading data into the module
    */

    uint8_t bufferId[1] = {1};
    uint8_t byte_buffer = 0;
    uint32_t timeout = 20000;
    uint32_t start_time = millis();
    uint8_t serial_buffer[534] = {0xff};
    uint16_t serial_buffer_length = 0;

    sendPacket(PID_COMMAND, CMD_UPLOAD_TEMPLATE, bufferId, 1);

    while(serial_buffer_length < 534 && (millis() - start_time) < timeout){
        if(Serial.available()){
            serial_buffer[serial_buffer_length++] = Serial.read();
        }
    }

    if(serial_buffer_length == 0){
        Serial.println("received no response updload Temp");
        return false;
    }

    if(serial_buffer_length < 10){
        Serial.println("bad packet nigga upload temp");
        return false ;
    }

    Serial.print("Serial buffer length: "); Serial.println(serial_buffer_length);

    for(uint16_t i = 0; i < serial_buffer_length; i++){
        Serial.print(serial_buffer[i], HEX);
    }
    Serial.println();

    return true;
}

bool BMA::readTemplateFromLib(){
    /*
        read template from library and load into charbuffer 1 and 2, then upload to upper computer
    */
    uint8_t data[3];
    uint16_t page_id = 3;
    uint8_t buffer_id = 1;

    data[0] = buffer_id;
    // data[1] = page_id & 0xff;
    // data[2] = (page_id >> 8) & 0xff;
    data[1] = 0x00;
    data[2] = 0x03;

    sendPacket(PID_COMMAND, CMD_READ_TEMPLATE, data, 3);

    uint8_t rx_response = receivePacket();
    if(rx_response != 0x00){
        Serial.println("readTemplateFromLib returned false");
        Serial.print("Confirmation Code: "); Serial.println(rx_response, HEX);
        return false;
    }
    Serial.println("Calling uploadTemplate");
    return uploadTemplate();
}