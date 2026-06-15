#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

typedef struct { float celsius; } temp_packet_t;
temp_packet_t incoming;

// ------ 7 SEGMENT DISPLAY SETUP -------- //
typedef struct seven_segment {
  int digits[4];
  int segments[8];
  int counter;
} seven_seg_t;

seven_seg_t Seven_Segment(int d0,int d1,int d2,int d3,int s0,int s1,int s2,int s3,int s4,int s5,int s6,int s7){
  seven_seg_t display;
  display.digits[0]=d0; display.digits[1]=d1; display.digits[2]=d2; display.digits[3]=d3;
  display.segments[0]=s0; display.segments[1]=s1; display.segments[2]=s2; display.segments[3]=s3;
  display.segments[4]=s4; display.segments[5]=s5; display.segments[6]=s6; display.segments[7]=s7;
  display.counter=0;
  return display;
}

void light_segment(seven_seg_t display,int digit,int segment){
  digitalWrite(display.digits[digit], HIGH);
  pinMode(display.segments[segment], OUTPUT);
  digitalWrite(display.segments[segment], LOW);
}

void clear_all(seven_seg_t display){
  for(int i=0;i<4;i++){ pinMode(display.digits[i],OUTPUT); digitalWrite(display.digits[i],LOW);}
  for(int i=0;i<8;i++){ pinMode(display.segments[i],INPUT);}
}

void display_number(seven_seg_t display,int digit,int number){
  if(number==1){light_segment(display,digit,1);light_segment(display,digit,2);}
  else if(number==2){light_segment(display,digit,0);light_segment(display,digit,1);light_segment(display,digit,3);light_segment(display,digit,4);light_segment(display,digit,6);}
  else if(number==3){light_segment(display,digit,0);light_segment(display,digit,1);light_segment(display,digit,2);light_segment(display,digit,3);light_segment(display,digit,6);}
  else if(number==4){light_segment(display,digit,1);light_segment(display,digit,2);light_segment(display,digit,5);light_segment(display,digit,6);}
  else if(number==5){light_segment(display,digit,0);light_segment(display,digit,2);light_segment(display,digit,3);light_segment(display,digit,5);light_segment(display,digit,6);}
  else if(number==6){light_segment(display,digit,0);light_segment(display,digit,2);light_segment(display,digit,3);light_segment(display,digit,4);light_segment(display,digit,5);light_segment(display,digit,6);}
  else if(number==7){light_segment(display,digit,0);light_segment(display,digit,1);light_segment(display,digit,2);}
  else if(number==8){light_segment(display,digit,0);light_segment(display,digit,1);light_segment(display,digit,2);light_segment(display,digit,3);light_segment(display,digit,4);light_segment(display,digit,5);light_segment(display,digit,6);}
  else if(number==9){light_segment(display,digit,0);light_segment(display,digit,1);light_segment(display,digit,2);light_segment(display,digit,3);light_segment(display,digit,5);light_segment(display,digit,6);}
  else if(number==0){light_segment(display,digit,0);light_segment(display,digit,1);light_segment(display,digit,2);light_segment(display,digit,3);light_segment(display,digit,4);light_segment(display,digit,5);}
}

void show_float(seven_seg_t* disp, int valueX10){
  bool leading = true;
  for(int pos=3; pos>=0; pos--){
    for(int s=0;s<8;s++){ pinMode(disp->segments[s], OUTPUT); digitalWrite(disp->segments[s], HIGH); }
    clear_all(*disp);

    int n=valueX10; for(int i=0;i<pos;i++) n/=10;
    int digit=n%10;

    bool forceShow = (pos <= 1);
    if(forceShow || !(digit==0 && leading)){
      leading=false;
      display_number(*disp, 3-pos, digit);
      // DP omitted (ghosts onto all digits). 
      if(pos==1) light_segment(*disp, 3-pos, 7);
    }
    delay(4);
  }
}
// ----------- END 7 SEGMENT DISPLAY SETUP ----------- //

seven_seg_t display;
volatile int tempX10 = 0;

void onReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len){
  if(len == sizeof(temp_packet_t)){
    memcpy((void*)&incoming, data, sizeof(incoming));
    tempX10 = (int)lround(incoming.celsius * 10.0);
    Serial.print("RX temp: "); Serial.println(incoming.celsius, 1);
  }
}

void setup(){
  Serial.begin(115200);
  delay(500);

  display = Seven_Segment(33, 27, 14, 18, 25, 12, 4, 2, 15, 26, 5, 0);
  clear_all(display);

  WiFi.mode(WIFI_STA);
  if(esp_now_init() != ESP_OK){ Serial.println("ESP-NOW init failed"); return; }
  esp_now_register_recv_cb(onReceive);
  Serial.println("Receiver ready - waiting for temp");
}

void loop(){
  int show = tempX10; if(show < 0) show = 0; if(show > 9999) show = 9999;
  show_float(&display, show);
}