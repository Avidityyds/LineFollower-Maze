#define F_CPU 8000000UL // CPU 時脈 8 MHz
#include <avr/io.h>              
#include <util/delay.h>          
#include <stdlib.h>              
#include <string.h>              

uint16_t ADCRead(const int); // 讀取指定 ADC 指定通道，回傳 10 位元數值
void USART_putstring(char* StringPtr); // 透過 UART 送字串

int main(void)
{
  // 系統時脈設定 
  CLKPR=(1<<CLKPCE); // 開啟時脈分頻修改
  CLKPR=0; // 設為不分頻，系統時脈 = 8 MHz
  
  // I/O 腳位方向設定
  DDRD=0xFF; // PORTD 全部設為輸出（PWM 輸出、UART TX）
  DDRB=0xFF; // PORTB 全部輸出
  DDRC = 0; // PORTC 全部輸入（感測器）

  // ADC 啟動
  ADCSRA |= (1<<ADEN);           
  
  // UART 初始化 
  unsigned int BaudR = 9600; // 設定鮑率 9600 bps
  unsigned int ubrr = (F_CPU / (BaudR*16UL))-1; // 計算 UBRR 值
  UBRR0H = (unsigned char)(ubrr>>8);
  UBRR0L = (unsigned char)ubrr;
  UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00); // 資料格式：8-bit
  UCSR0B |= (1<<TXEN0); // 啟用傳送端 (TX)

  // Timer0 / Timer2 設定成 Fast PWM，控制馬達
  TCCR0A = 0b10100011; // Fast PWM 模式，OC0A/OC0B 非反相
  TCCR0B = 0b00000101; // 預分頻 1024
  TCCR2A = 0b10100011; // Fast PWM 模式，OC2A/OC2B 非反相
  TCCR2B = 0b00000101; // 預分頻 1024

  while (1)
  {
    // 感測器取樣：左 / 中 / 右
    float IR_left=0, IR_mid=0, IR_right=0;
    for(int i = 0; i < 10; i++){       
      IR_left  += (float)ADCRead(0b00000101); 
      IR_mid   += (float)ADCRead(0b00000100); 
      IR_right += (float)ADCRead(0b00000011); 
    }
    IR_left  /= 10;
    IR_mid   /= 10;
    IR_right /= 10;

    // P 控制計算 
    const float KP = 0.09;       // 比例增益常數
    float error = IR_right - IR_left; // 誤差 = 右感測值 - 左感測值
    float P_value = KP * error;  // P 控制輸出值

    // 計算左右輪速度
    float Left_speed  = 95 + P_value;
    if (Left_speed > 255) Left_speed = 255; // 限制上限
    if (Left_speed < 30)  Left_speed = 20;  // 限制下限

    float Right_speed = 85 - P_value;
    if (Right_speed > 255) Right_speed = 255;
    if (Right_speed < 30)  Right_speed = 20;

    // 串列輸出左右輪速度（debug 用）
    char Left[8],Right[8];
    char *intStr4 = itoa((int)Left_speed, Left, 10); 
    strcat(intStr4,"Left\n");                        
    USART_putstring(intStr4);                       

    char *intStr6 = itoa((int)Right_speed, Right, 10); 
    strcat(intStr6,"Right\n");
    USART_putstring(intStr6);

    // 馬達輸出決策 
    if (abs(error) < 30){ // 誤差很小 → 直接直行
      OCR0A=90;                  
      OCR0B=0;
      OCR2A=0;
      OCR2B=100;                
    }
    else if (error < 0){ // error < 0 → 左邊比較強 → 車偏左 → 需要往左修正
      OCR0A=Right_speed;         
      OCR0B=0;
      OCR2A=0;
      OCR2B=Left_speed;         
    }
    else if (error > 0){ // error > 0 → 右邊比較強 → 車偏右 → 需要往右修正
      OCR0A=Right_speed;
      OCR0B=0;
      OCR2A=0;
      OCR2B=Left_speed;
    }
  }
}

// 讀取指定 ADC 指定通道（感測器）
uint16_t ADCRead(const int channel) {
  ADMUX = 0b01000000; // 參考電壓 AVcc，結果右對齊
  ADMUX |= channel; // 選擇通道
  ADCSRA |= (1<<ADSC) | (1<<ADIF); // 開始轉換，並清 ADIF
  while ( (ADCSRA & (1<<ADIF)) == 0); // 等待轉換完成
  ADCSRA &= ~(1<<ADSC); // 清 ADSC
  return ADC;                     
}

// 串列送字串
void USART_putstring(char* StringPtr){
  while(*StringPtr != 0x00){      // 一直到 '\0' 結束
    while(!(UCSR0A & (1<<UDRE0)));// 等待傳送暫存器空
    UDR0 = *StringPtr;            // 傳送一個字元
    StringPtr++;                  // 下一個字元
  }
}
