/*
 * Atmegalodon.cpp
 *
 * 
 * Author : svetota
 */ 

#define ICR_MAX F_CPU/50  // ICR1(TOP) = fclk/(N*f) ; N-��������; f-�������;  1000000/1/50 = 20000
#define OCR_MIN ICR_MAX/20
#define OCR_MAX ICR_MAX/10
#define OCR_CENTER (ICR_MAX/4/10)*3


#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


class PortLed		// ����� ������ ��� ������ �� ������������
{
	public:
	virtual int State(void)		// ��������� ���������� � ��������� � ���� ������ (0/1)
	{
		return 2;		// �������� ��� ����������� �������
	}
	const char* StateText(void)		// ��������� ���������� � ��������� � ���� ������
	{
		char message[20] = "LED is now ";
		//strcat(message, "LED is now ");
		switch (this->State())
		{
			case 0 : {strcat(message, "OFF"); break;}
			case 1 : {strcat(message, "ON"); break;}
			case 2 : {strcat(message, "undefined"); break;}
		}
		const char* endmessage = message;		// �� ���� ����� ����������� �� ��������� � char*
		return endmessage;
	}
};
class PortLedB0 : public PortLed		// ��� ������ �� ����������� �� ���� 0 ����� B
{
	public:
		PortLedB0(void)		// ������������� � ������������
		{
			DDRB |= (1<<0);		// ����������� ��� B0 � ����� ������
		}
		int On(void)		// ���������
		{
			PORTB |= (1<<0);		// ����� ������� ������� �� ��� B0
			return 0;
		}
		int Off(void)		// ����������
		{
			PORTB &= ~(1<<0);		// ������ ������� �� ��� B0
			return 0;
		}
		int State(void)		// ��������� ���������� � ��������� � ���� ������ (0/1)
		{
/*!*/		return (PINB & (1<<0));		// ����� �� �������� PINB! � ��������� ����� �� ���������, ����������!
		}
};

class PortUart		// ����� ��� ������ RS232 �� ������� ������ RXD/TXD (PD0/PD1)
{
	public:
		PortUart(void)		// ������������� � ������������
		{
			UCSRB |= (1<<3) | (1<<4);		// �������� � �������� UCRSB ���������� (3-� ���, PD1->TXD) � ������� (3-� ���, PD0->RXD) UART
			UCSRC = 0x86;		// ������������� ����� ����� 8 ���, ���� 1 ���, �������� ����
			UCSRC &= ~(1<<7);	// ������������� �� ������ � UBRR
			UBRRH = 0x00;		//
/*!*/		UBRRL =	0x33;		// ������������� � �������� UBBR �������� �������� 9600 (��� 8 ���) �� ������� ������ ���� 51, �� � �������� �������� ���?!
		}
		int Send(const char* text)		// ����������� ������
		{
			strcat( (char*)text, "\r\n");
			for(unsigned int i = 0; i < strlen(text); i++)
			{
				while(!(UCSRA & (1<<5))) {}		// ��������� ����������� ��� UDRE, ���� ������ ����������, ���
				UDR = text[i];		// ����� �����, �������� ������
			}
			return 0;
		}
		int Send(int number)		// ����������� �����
		{
			//strcat( (char*)text, "\r\n");
			char buff[10] = "123456789";
			char* text = itoa(number, buff, 10);	// ��������� � �����
			for(unsigned int i = 0; i < strlen(text); i++)	// unsigned ��� strlen
			{
				while(!(UCSRA & (1<<5))) {}		// ��������� ����������� ��� UDRE, ���� ������ ����������, ���
				UDR = text[i];		// ����� �����, �������� ������
			}
			return 0;
		}
		int Get(void)		// ��������� ����, ���� ������� �����, ������ ���� (�������� ��������?)
		{
			while(1)
			{
				if(UCSRA & (1<<7))
				{
					char zero[2] = "0";			//
					char* symbol = zero;		// �� ���� ����� ����������� �� ��������� � char* (deprecated conversion)
					symbol[0] = UDR;
					return atoi(symbol);
				}		// ��������� � ����� ��� RXC, ����� �� ��������, ���� �����, ������ ������
			}
		}
		int GetCoord(void)		// ���������� �������������� ����������
		{
			this->Send("\n\rReady to get coordinate...\r\n");
			char zero[5] = "0000";	//
			char* coord = zero;		// �� ���� ����� ����������� �� ��������� � char* (deprecated conversion)
			int intcoord = 0;
			for(int i = 0;i < 4; )		// �������� � ����� ������ �������, ��������� ������� ���������� ������� Get
			{
				if(UCSRA & (1<<7))
				{
					coord[i] = UDR;
					i++;
				
				}
			}
			this->Send("Done.\r\n");
			intcoord = atoi(coord);		// ��������� ������ � ����� �����
			return intcoord;
		}
		
};

class PortServo		//��� ������ � �������������� �� ����� ����������� ��� (PB1/OC1A, PB2/OC1B)
{
	public:
		PortServo(int portnumber)
		{
				// ����������� PWM �� ������� 1 (����� �� ����� PB1, PB2)
				TCCR1A = 0; // ��������� PWM ���� ����� ���������������
				ICR1 = (unsigned int) ICR_MAX; // ������� ������ 50 ��
				// �������� Fast PWM mode via ICR1 �� ������� 1 ��� �������� �������
				TCCR1A = (1<<WGM11);
				TCCR1B = (1<<WGM13) | (1<<WGM12) | (1<<CS10);
				
				// ������������ PB1 � PB2 ��� �����
				DDRB |= (1<<1) | (1<<2);
				
				// �������� PWM �� port B1 � B2
				TCCR1A |= (1<<COM1A1) | (1<<COM1B1);
				
				// �������� ���
				this->AdcInit();
			if(portnumber == 1) _delay_ms(500);
		}
		void SetCoord(int coord)
		{
			OCR1A = round(OCR_MIN + coord * (OCR_MAX - OCR_MIN) / 1800);
			OCR1B = round(OCR_MIN + coord * (OCR_MAX - OCR_MIN) / 1800);
			
		}

		private:

		void AdcInit(void)	// ������������� ���
		{
			ADCSRA = _BV(ADEN) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // prescaler = 128
		}

		uint32_t AdcRead(uint8_t ch)	// ������ ������ ���
		{
			ADMUX = _BV(REFS0) | (ch & 0x1F);   // set channel (VREF = VCC)
			ADCSRA &= ~_BV(ADIF);           // clear hardware "conversion complete" flag
			ADCSRA |= _BV(ADSC);            // start conversion
			while(ADCSRA & _BV(ADSC));      // wait until conversion complete
			return ADC;             // read ADC (full 10 bits);
		}

};


int main(void)
{
	PortLedB0 Led;		//
	PortUart Uart;		// ���������� ����������
	PortServo Servo(1); //
	
    while (1)		// ������� ����
    {
		/*����������� ��������*/
		Uart.Send(Led.StateText());
		Uart.Send("\r\n\rCommand please\n\r");
		switch (Uart.Get())
		{
			case 0 : {Led.Off(); break;}
			case 1 : {Led.On(); break;}
		}
	}
	
}