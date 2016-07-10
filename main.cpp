/*
 * Atmegalodon.cpp
 *
 * 
 * Author : svetota
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


class PortLed		// общие методы для работы со светодиодами
{
	public:
	virtual int State(void)		// выдавалка информации о состоянии в виде целого (0/1)
	{
		return 2;		// заглушка для производных классов
	}
	const char* StateText(void)		// выдавалка информации о состоянии в виде строки
	{
		char message[20] = "LED is now ";
		//strcat(message, "LED is now ");
		switch (this->State())
		{
			case 0 : {strcat(message, "OFF"); break;}
			case 1 : {strcat(message, "ON"); break;}
			case 2 : {strcat(message, "undefined"); break;}
		}
		const char* endmessage = message;		// не смог иначе вывернуться из ворнингов с char*
		return endmessage;
	}
};
class PortLedB0 : public PortLed		// для работы со светодиодом на пине 0 порта B
{
	public:
		PortLedB0(void)		// инициализация в конструкторе
		{
			DDRB |= (1<<0);		// переключаем пин B0 в режим выхода
		}
		int On(void)		// включалка
		{
			PORTB |= (1<<0);		// подаём высокий уровень на пин B0
			return 0;
		}
		int Off(void)		// выключалка
		{
			PORTB &= ~(1<<0);		// низкий уровень на пин B0
			return 0;
		}
		int State(void)		// выдавалка информации о состоянии в виде целого (0/1)
		{
/*!*/		return (PINB & (1<<0));		// берем из регистра PINB! С НЕНУЛЕВОЙ НОГОЙ НЕ СРАБОТАЕТ, ПЕРЕДЕЛАТЬ!
		}
};

class PortUart		// класс для работы RS232 на штатных портах RXD/TXD (PD0/PD1)
{
	public:
		PortUart(void)		// инициализация в конструкторе
		{
			UCSRB |= (1<<3) | (1<<4);		// включаем в регистре UCRSB передатчик (3-й бит, PD1->TXD) и приёмник (3-й бит, PD0->RXD) UART
			UCSRC = 0x86;		// устанавливаем длину слова 8 бит, стоп 1 бит, чётность откл
			UCSRC &= ~(1<<7);	// переключаемся на доступ к UBRR
			UBRRH = 0x00;		//
/*!*/		UBRRL =	0x33;		// устанавливаем в регистре UBBR скорость передачи 9600 (при 8 МГц) ПО ФОРМУЛЕ ДОЛЖНО БЫТЬ 51, НО В ПРОТЕУСЕ РАБОТАЕТ ТАК?!
		}
		int Send(const char* text)		// отправлялка текста
		{
			strcat( (char*)text, "\r\n");
			for(unsigned int i = 0; i < strlen(text); i++)
			{
				while(!(UCSRA & (1<<5))) {}		// проверяем контрольный бит UDRE, если нельзя передавать, ждём
				UDR = text[i];		// когда можно, отсылаем символ
			}
			return 0;
		}
		int Send(int number)		// отправлялка чисел
		{
			//strcat( (char*)text, "\r\n");
			char buff[10] = "123456789";
			char* text = itoa(number, buff, 10);	// переводим в текст
			for(unsigned int i = 0; i < strlen(text); i++)	// unsigned под strlen
			{
				while(!(UCSRA & (1<<5))) {}		// проверяем контрольный бит UDRE, если нельзя передавать, ждём
				UDR = text[i];		// когда можно, отсылаем символ
			}
			return 0;
		}
		int Get(void)		// получалка цифр, если введена буква, выдает ноль (ВОЗМОЖНЫ ВАРИАНТЫ?)
		{
			while(1)
			{
				if(UCSRA & (1<<7))
				{
					char zero[2] = "0";			//
					char* symbol = zero;		// не смог иначе вывернуться из ворнингов с char* (deprecated conversion)
					symbol[0] = UDR;
					return atoi(symbol);
				}		// проверяем в цикле бит RXC, можно ли получать, если можно, читаем символ
			}
		}
		int GetCoord(void)		// получатель четырехзначной координаты
		{
			this->Send("\n\rReady to get coordinate...\r\n");
			char zero[5] = "0000";	//
			char* coord = zero;		// не смог иначе вывернуться из ворнингов с char* (deprecated conversion)
			int intcoord = 0;
			for(int i = 0;i < 4; )		// получаем в цикле четыре символа, получение каждого аналогично функции Get
			{
				if(UCSRA & (1<<7))
				{
					coord[i] = UDR;
					i++;
				
				}
			}
			this->Send("Done.\r\n");
			intcoord = atoi(coord);		// переводим строку в целое число
			return intcoord;
		}
};

class PortServo		//для работы с сервоприводами на пинах аппаратного ШИМ (PB1/OC1A, PB2/OC1B)
{
	public:
		PortServo(int portnumber)
		{
				// Настраиваем PWM на таймере 1 (выход на ногах PB1, PB2)
				TCCR1A = 0; // Отключаем PWM пока будем конфигурировать
				ICR1 = (unsigned int) ICR_MAX; // Частота всегда 50 Гц
				// Включаем Fast PWM mode via ICR1 на Таймере 1 без делителя частоты
				TCCR1A = (1<<WGM11);
				TCCR1B = (1<<WGM13) | (1<<WGM12) | (1<<CS10);
				
				// Устанавливае PB1 и PB2 как выход
				DDRB |= (1<<1) | (1<<2);
				
				// Включаем PWM на port B1 и B2
				TCCR1A |= (1<<COM1A1) | (1<<COM1B1);
				
				// Включаем АЦП
				this->AdcInit();
			if(portnumber == 1) _delay_ms(500);
		}
		void SetCoord(int coord)
		{
			OCR1A = round(OCR_MIN + coord * (OCR_MAX - OCR_MIN) / 1800);
			OCR1B = round(OCR_MIN + coord * (OCR_MAX - OCR_MIN) / 1800);
			
		}

		private:
	
		const unsigned int ICR_MAX = (unsigned int) F_CPU/50;  // ICR1(TOP) = fclk/(N*f) ; N-Делитель; f-Частота;  1000000/1/50 = 20000
		const unsigned int OCR_MIN = ICR_MAX/20;
		const unsigned int OCR_MAX = ICR_MAX/10;
		const unsigned int OCR_CENTER = (ICR_MAX/4/10)*3;

		void AdcInit(void)	// Инициализация АЦП
		{
			ADCSRA = _BV(ADEN) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // prescaler = 128
		}

		uint32_t AdcRead(uint8_t ch)	// Чтение канала АЦП
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
	PortUart Uart;		// подключаем интерфейсы
	PortServo Servo(1); //
	
    while (1)		// главный цикл
    {
		/*выполняемые действия*/
		Uart.Send(Led.StateText());
		Uart.Send("\r\n\rC o m m a n d   p l e a s e\n\r");
		switch (Uart.Get())
		{
			case 0 : {Led.Off(); break;}
			case 1 : {Led.On(); break;}
		}
	}
	
}