#include <Arduino.h>
#include "TM1640.h"

// SCLK = 2, DIN = A0
TM1640 LED(2, A0);
uint8_t AllOff[16] ={0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00};
uint8_t DispayDuty=TM1640_DUTY_14_16;

void setup() {
  Serial.begin(230400);
  LED.SetDuty(DispayDuty);
  delay(100);
}

void loop() {
  uint8_t buff[16];
  LED.DrawAddrInc(AllOff,16);
  delay(10);
  //Red
  buff[0] =0b10000000;  //SEG1
  buff[1] =0b11000000;  //SEG2
  buff[2] =0b11100000;  //SEG3
  buff[3] =0b11110000;  //SEG4
  buff[4] =0b11111000;  //SEG5
  buff[5] =0b11111100;  //SEG6
  buff[6] =0b11111110;  //SEG7
  buff[7] =0b11111111;  //SEG8
  //Green
  buff[8] =0b00000001;  //SEG1
  buff[9] =0b00000011;  //SEG2
  buff[10]=0b00000111;  //SEG3
  buff[11]=0b00001111;  //SEG4
  buff[12]=0b00011111;  //SEG5
  buff[13]=0b00111111;  //SEG6
  buff[14]=0b01111111;  //SEG7
  buff[15]=0b11111111;  //SEG8
  LED.DrawAddrInc(buff, 16);
  delay(1000);

  buff[0] =0b11111111;  //SEG1　
  buff[1] =0b01111111;  //SEG2
  buff[2] =0b00111111;  //SEG3
  buff[3] =0b00011111;  //SEG4
  buff[4] =0b00001111;  //SEG5
  buff[5] =0b00000111;  //SEG6
  buff[6] =0b00000011;  //SEG7
  buff[7] =0b00000001;  //SEG8
  buff[8] =0b11111111;  //SEG1
  buff[9] =0b11111110;  //SEG2
  buff[10]=0b11111100;  //SEG3
  buff[11]=0b11111000;  //SEG4
  buff[12]=0b11110000;  //SEG5
  buff[13]=0b11100000;  //SEG6
  buff[14]=0b11000000;  //SEG7
  buff[15]=0b10000000;  //SEG8
  LED.DrawAddrInc(buff, 16);
  delay(1000);

  for(uint8_t i=0; i<16; i++){
    buff[i] =0b00000000;  //SEG1  
  }
  LED.DrawAddrInc(buff, 16); 

  //FPS benchi
  unsigned long ss = micros();
  for(uint8_t times=0; times<10; times++){
    for(uint8_t i = 0; i < 16; i++){
      uint8_t bit = 0b10000000;
      for(uint8_t j = 0; j < 8; j++){
        buff[i] = bit;
        LED.DrawAddrInc(buff, 16);
        bit >>= 1;
      }
      buff[i] = 0;
    }
  }
  ss = micros() - ss;
  Serial.println("Score:");
  Serial.print(ss);Serial.println(" us");
  float fps = 1280.0f * 1000000.0f / ss;
  Serial.print("FPS: ");Serial.print(fps, 1);  Serial.println(" FPS");
  float bitrate_kbps = (272.0f * 1280.0f) / (ss / 1000.0f);  // 272bit * 1280
  Serial.print("Bitrate: "); Serial.print(bitrate_kbps, 1); Serial.println(" kbps");

  //Duty change 
  LED.SetDuty(DispayDuty);
  DispayDuty ++;
  if(DispayDuty > 8) DispayDuty = 0;
}
