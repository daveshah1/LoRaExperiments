#include <SPI.h>

#include <RFM98W_library.h>
RFMLib radio = RFMLib(PB12, PC6, 16, 21);
#define nss PB12

const uint8_t packet_size = 254;
uint8_t curr_packet[packet_size];
uint8_t curr_packet_len = 0;

void setup() {
  Serial.begin(115200);
  //Comment out the below line for most platforms
  SPI.setModule(2);
  SPI.begin();
  byte my_config[6] = {0x84, 0x74, 0x85, 0xAF, 0xCD, 0x00};
  radio.high_frequency = true;
  radio.configure(my_config);
  radio.setFrequency(869525000);

} 

void RFMISR() {
  radio.rfm_done = true;
}


const uint8_t xon = 17;
const uint8_t xoff = 19;
bool is_xoff = false;

bool is_our_turn = true;
bool in_rx = false;
long last_active_time = 0;
long turn_timeout = 1500;

const uint8_t magic = 0x42;


void send_xoff() {
  if(!is_xoff) {
    Serial.write(xoff);
    is_xoff = true;
  }
}

void send_xon() {
  if(is_xoff) {
    Serial.write(xon);
    is_xoff = false;
  }
}

void loop() {
  while(Serial.available() && (curr_packet_len < packet_size)) {
    uint8_t b = Serial.read();
    if((b != xon) && (b != xoff))
      curr_packet[curr_packet_len++] = b;
  }
  
  if(curr_packet_len >= packet_size) {
    send_xoff();
  }

  //Are we allowed to TX?
  if(is_our_turn || ((millis() - last_active_time) > turn_timeout)) {
    //terminate pending rx
    if(in_rx) {
       RFMLib::Packet dummy;
       radio.endRX(dummy); 
       in_rx = false;
    }
    RFMLib::Packet tx;
    tx.len = curr_packet_len + 1;
    tx.data[0] = magic;
    
    for(int i = 0; i < curr_packet_len; i++)
      tx.data[i + 1] = curr_packet[i];
      
    radio.beginTX(tx);
    attachInterrupt(PC6,RFMISR,RISING);

    curr_packet_len = 0;
    is_our_turn = false;
    last_active_time = millis();

    send_xon();
  } else if(radio.rfm_status == 0) {
    in_rx = true;
    radio.beginRX();
    attachInterrupt(PC6, RFMISR, RISING);    
  }

  if(radio.rfm_done) {
    if(in_rx) {
      RFMLib::Packet rx;
      radio.endRX(rx);
      for(int i = 1; i < rx.len; i++) {
        Serial.write(rx.data[i]);
      }
      in_rx = false;
      is_our_turn = true;
      last_active_time = millis();
      
    } else {
      radio.endTX();
    }
  }
}


