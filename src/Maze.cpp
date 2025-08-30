#define F_CPU 8000000UL // CPU 時脈 8 MHz
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

uint16_t ADCRead(const int); // 讀取指定 ADC 通道，回傳 10-bit 數值
void USART_putstring(char* StringPtr); // 透過 UART 送出字串（結尾需為'\0'）

int main(void)
{
    // 系統時脈預分頻設定 
	CLKPR = (1<<CLKPCE); // 允許改寫時脈預分頻器
	CLKPR = 0; // 將預分頻設為 1：系統時脈 = F_CPU = 8 MHz

    // I/O 方向設定 
	DDRD = 0xFF; // PORTD 全部設為輸出（PWM、UART TX）
	DDRB = 0xFF; // PORTB 全部輸出（馬達驅動）
	DDRC = 0; // PORTC 全部輸入（感測器）

    // Timer0 / Timer2：Fast PWM 設定（控制兩顆馬達的正反轉 PWM）
	TCCR0A=0b10100011; // Fast PWM；OC0A/OC0B 
	TCCR0B=0b00000105; // WGM02=0（Fast PWM 8-bit）、CS02:0=101 → clk/1024
	TCCR2A=0b10100011; // Timer2 同上：Fast PWM；OC2A/OC2B 非反相
	TCCR2B=0b00000101; // CS22:0=101 → clk/128

    // ADC 啟用
	ADCSRA |= (1<<ADEN);         

    // UART 初始化：9600 bps、8N1、只開 TX
	unsigned int BaudR = 9600;
	unsigned int ubrr = (F_CPU / (BaudR*16UL))-1; // UBRR 計算公式
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00); // 資料位元 8-bit（UCSZ01:0 = 11）
	UCSR0B |= (1<<TXEN0); // 只啟用傳送端（TX）

	int distanceR; // 右側距離（由讀取電壓代入公式換算）

	while(1)
	{	
		// 讀感測器並平均
		float sumValDMS = 0,sumValIR = 0; 
		for(int i = 0; i < 10; i++){
			sumValDMS += (float)ADCRead(0); // 讀 DMS 
			sumValIR  += (float)ADCRead(4); // 讀 IR 
		}
		sumValDMS /= 10;                  
		sumValIR  /= 10;

		// 將 ADC0 轉電壓，再用公式換算距離
		float voltage = sumValDMS * 5 / 1023;  // 0~1023 對應 0~5V
		distanceR = 29.98 * pow(voltage, -1.17); // 轉換公式

		// 串列輸出兩個數值：IR原始平均值、DMS距離 
		char BufferDMS[8], BufferIR[8];   
		char *intStrDMS = itoa((int)distanceR, BufferDMS, 10); 
		char *intStrIR  = itoa((int)sumValIR,  BufferIR, 10); 
		strcat(intStrIR,"_");             
		strcat(intStrDMS,"\n");           
		USART_putstring(intStrIR);        
		USART_putstring(intStrDMS);       

		// 馬達控制決策 
		if (distanceR <= 10){ // 左邊太近 → 往右轉（右輪煞、左輪快）
			OCR2A=220;        
			OCR2B=0;
			OCR0A=0;
			OCR0B=0;
		}
		else { // 安全距離 → 以前進（稍偏左）的動力推進
			OCR2A=50;         
			OCR2B=0;
			OCR0A=0;
			OCR0B=255;       
		}

		// 第二條件：若 IR <800，判定太靠近 → 倒退
		if (sumValIR < 800){ // 倒退（右輪較強，帶點右旋）
			OCR2A=0;
			OCR2B=50;       
			OCR0A=255;       
			OCR0B=0;
			_delay_ms(450); // 倒退 450ms 
		}
	}
}

// 讀取指定 ADC 通道數值（感測器）
uint16_t ADCRead(const int channel) {
	ADMUX = 0b01000000; // 參考電壓 AVcc（REFS0=1），結果右對齊（ADLAR=0），通道先清零
	ADMUX |= channel; // 選擇要讀的 ADC 通道（0~7）

	ADCSRA |= (1<<ADSC) | (1<<ADIF); // 啟動轉換（ADSC=1）；同時寫 1 來清除 ADIF 旗標
	while ( (ADCSRA & (1<<ADIF)) == 0); // 等待轉換完成（ADIF 置 1）
	ADCSRA &= ~(1<<ADSC); // 保險起見，清掉 ADSC

	return ADC;                   
}

// 透過 UART 送出以 '\0' 結尾的字串
void USART_putstring(char* StringPtr)
{
	while(*StringPtr != 0x00){ // 直到遇到字串結尾
		while(!(UCSR0A & (1<<UDRE0))); // 等待資料暫存器空（可寫）
		UDR0 = *StringPtr; // 丟一個字元出去
		StringPtr++; // 指向下一個字元
	}
}
