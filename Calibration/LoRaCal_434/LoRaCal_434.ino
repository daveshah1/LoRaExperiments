#define nss 27
#include "SPI.h"
void wRFM(byte ad, byte val);
void bwRFM(byte ad, byte vals[], int n);
byte rRFM(byte ad);
void brRFM(byte ad, byte vals[], byte len);
void setFrequency(uint32_t frequency);
void enterMode(uint32_t mode, bool lora = false);

//Module config
const uint32_t Fosc_nom = 32000000;
uint32_t Fosc = Fosc_nom;
bool low_frequency = false;
uint8_t LNAGain = 1; //Max LNA gain
uint8_t RSSIsamples = 0x06; //Average 128 samples for each RSSI result
uint8_t RxBw = 0x17; //Rx bandwidth control = 2.6kHz 

//Expected peak frequency
uint32_t expectedPeak = 432000000;
uint32_t sweepIncrement = 200;
uint32_t maxOffset = 50000;
float rssiThresh = -110;

void setup() {
  // put your setup code here, to run once:
    pinMode(26, OUTPUT);
  digitalWrite(26, HIGH);
  pinMode(nss,OUTPUT);
  digitalWrite(nss,HIGH);
  Serial.begin(115200);

 // SPI.setModule(2);
  SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV64); 

  //Serial.println(rRFM(0x42), HEX);
  enterMode(0, true); //sleep
  delay(5);
  enterMode(0, false); //sleep
  delay(5);
  enterMode(1); //standby
  delay(5);
      setFrequency(expectedPeak);

  wRFM(0x0e, RSSIsamples & 0x07); //no RSSI offset; x RSSI averaging samples
  wRFM(0x12, RxBw);
  wRFM(0x0D, 0x00); //disable AGC and other automatic crud
  wRFM(0x31, 0x00); //continuous not packet mode
  wRFM(0x14, 0x10); //disable bit sync
}

bool calibrate(uint32_t &measuredFreq, float &measuredRSSI, uint32_t &calibFosc) {
  uint32_t Fstart = expectedPeak - maxOffset;
  uint32_t Fend = expectedPeak + maxOffset;

  float peakRSSI = -129;
  uint32_t peakFreq = expectedPeak;
  
  for(uint32_t freq = Fstart; freq < Fend; freq += sweepIncrement) {
      enterMode(1); //standby
      setFrequency(freq);
      enterMode(4); //FSRx
      delay(5);
      enterMode(5); //Rx
      delay(100);
      float rssi = 0 - (int)(rRFM(0x11)) / 2.0;
      if(rssi > peakRSSI) {
        peakRSSI = rssi;
        peakFreq = freq;
      }
  }
   enterMode(1); //standby
  if(peakRSSI > rssiThresh) {

    measuredFreq = peakFreq;
    measuredRSSI = peakRSSI;
    calibFosc = (((uint64_t)Fosc * expectedPeak) / peakFreq);
    return true;
  } else {
   
    measuredFreq = peakFreq;
    measuredRSSI = peakRSSI;
    return false; 
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.available()) {
    char c = Serial.read();
    uint32_t measuredFreq, newFosc;
    float measuredRSSI;
    Serial.print("Starting calibration using ");
    Serial.print(expectedPeak * 1e-6);
    Serial.println("MHz reference");
    if(calibrate(measuredFreq, measuredRSSI, newFosc)) {
      Serial.println("Calibration successful");
      Serial.print("Peak of ");
      Serial.print(measuredRSSI);
      Serial.print("dBm found at ");
      Serial.print(measuredFreq);
      Serial.println("Hz");
      Serial.print("New Fosc = ");
      Serial.print(newFosc);
      Serial.print("Hz (");
      Serial.print(((int)newFosc - (int)Fosc) / (newFosc * 1e-6));
      Serial.println(" ppm offset)");
      Fosc = newFosc;
    } else {
      Serial.println("Calibration failed");
      Serial.print("Peak of ");
      Serial.print(measuredRSSI);
      Serial.print(" found at ");
      Serial.print(measuredFreq * 1e-6);
      Serial.println("MHz");
    }
  }
}

void enterMode(uint32_t mode, bool lora) {
  //OOK modulation; specified freq band
  wRFM(0x01, (lora << 7) | (0x01 << 5) | 0x08 | (mode & 0x07));
  //Serial.println(rRFM(0x01));
}

void setFrequency(uint32_t frequency) {
    uint32_t freqVal = ((uint64_t)(frequency) << 19ULL) / Fosc;

    wRFM(0x06, (freqVal >> 16) & 0xFF);
    wRFM(0x07, (freqVal >> 8) & 0xFF);
    wRFM(0x08, (freqVal) & 0xFF);

}

void wRFM(byte ad, byte val) {//single byte write
    digitalWrite(nss,LOW);
    SPI.transfer(ad | 128);//set wrn bit - WRITE = 1
    SPI.transfer(val);
    digitalWrite(nss,HIGH);
};


void bwRFM(byte ad, byte vals[], int n){//burst write - less efficient but faster
    //for multiple bits
    //(less efficient for singles due to array overhead etc)
    digitalWrite(nss,LOW);
    SPI.transfer(ad | 128);//set wrn bit - WRITE = 1
    for(int i = 0;i<n;i++)
        SPI.transfer(vals[i]);

    digitalWrite(nss,HIGH);
};

byte rRFM(byte ad){//single byte read
    digitalWrite(nss,LOW);
    SPI.transfer(ad & B01111111);//wrn bit low
    byte val = SPI.transfer(0);//read, but we still have to spec a value?
    digitalWrite(nss,HIGH);
    return val;
};

void brRFM(byte ad, byte vals[], byte len){//burst read - slower for singles due to
    digitalWrite(nss,LOW);   //overhead of working with arrays
    SPI.transfer(ad & 0x7F);//wrn bit low
    for(int i = 0;i<len;i++){
        vals[i] = SPI.transfer(0);
    }

   digitalWrite(nss,HIGH);
};
