#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <FreqCount.h> 
#include <PWM.h>
#include <CyberLib.h>
#define led  9       // пин для генератора сигналов (не менять)
#define dds  10      // пин для генератора dds (не менять)
#define key_down 11  // кнопка минус (можно любой пин)
#define key_ok   12  // кнопка ОК (можно любой пин)
#define key_up   13  // кнопка плюс (можно любой пин)
#define akb A5       // любой свободный аналоговый пин для измерения напряжения АКБ 
#define overclock 16 // частота на которой работает Ардуино
#define contrast 40  // контрастность дисплея
//    int SCK, int MOSI, int DC, int RST, int CS
// (clk(scl), din(sda), dc, ce, rst)       
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 4, 3, 2); // пины к которым у вас подключен дисплей        
byte SinU = 30;  // уровень синхронизации 0 до 255 
int PWM = 50; // стартовое значение ШИМ от 0 до 100        
int32_t freq = 500; //стартовое значение частоты в Гц
float VCC = 5.0;  //напряжение питания, меряем мультиметром
				  //###########################################################
int d = 0;
const byte sinM[]     PROGMEM = { 1,6,15,29,48,69,92,117,143,168,191,212,229,243,251,255,254,248,237,222,203,181,156,131,106,81,59,39,22,10,3,1 };
const byte pilaM[]    PROGMEM = { 1,9,17,25,33,41,49,57,65,73,81,89,97,105,113,121,129,137,145,153,161,169,177,185,193,201,209,217,225,235,245,255 };
const byte RpilaM[]   PROGMEM = { 250,246,238,230,222,214,206,198,190,182,174,166,158,150,142,134,126,118,110,102,94,86,78,70,62,54,41,33,25,17,9,1 };
const byte trianglM[] PROGMEM = { 1,18,35,52,69,86,103,120,137,154,171,188,205,222,239,255,239,223,207,191,175,159,143,127,111,95,79,63,47,31,15,1 };
byte hag = 0;
int stepFreq = 0;
boolean flag = 0;
byte mass[701];
byte x = 0;
byte ddsMenu = 0; // меню dds-генератора 
byte oscMenu = 0; // меню осциллоскопа 
bool opornoe = 1; // флаг опорного напряжения
bool pause = 0; // флаг режима паузы
byte mainMenu = 0; // режим работы прибора 
byte razv = 6;
unsigned long count = 0;
byte sinX = 30;
byte meaX = 83;
int Vmax = 0;// максимальное напряжение 
byte sinhMASS = 0;
long countX = 0;
long speedTTL = 57600; // скорость терминала по умолчанию 
int  prokr = 0;
bool flag_key; // нужно для разных подключений кнопок
int adcBat;

void setup() {
	byte key_test = 0;
	// подтянули кнопки к питанию
	digitalWrite(key_up, HIGH); digitalWrite(key_down, HIGH); digitalWrite(key_ok, HIGH);
	// автодетект подключения кнопок при включении..
	if (digitalRead(key_up))   key_test++; // если подтяжка к питанию осталась (не стоят резюки на массу) - плюсуем
	if (digitalRead(key_down)) key_test++; // если подтяжка к питанию осталась (не стоят резюки на массу) - плюсуем
	if (digitalRead(key_ok))   key_test++; // если подтяжка к питанию осталась (не стоят резюки на массу) - плюсуем
	if (key_test > 1) { // определили подключение кнопок на массу
		flag_key = 0;
	}
	else { // определили подключение кнопок на питание + резюки на массу
		digitalWrite(key_up, LOW); digitalWrite(key_down, LOW); digitalWrite(key_ok, LOW); // убрали подтяжку 
		flag_key = 1;
	}
	// если автодетект работает неверно (менюха скачет постоянно), то нужно расскомментировать одну из следующих строчек:
	// flag_key = 0; // кнопки просто подключены к земле, резюков нету.
	// flag_key = 1; // кнопки подключены к питанию и резюки на землю

	// работаем с дисплеем...     
	display.begin();
	display.setContrast(contrast);
	display.clearDisplay();
	display.display();
	while (flag_key - digitalRead(key_ok)) { // выводим меню, пока не выбран режим работы прибора  
		display.clearDisplay();
		if (mainMenu == 0)   display.setTextColor(WHITE, BLACK); else display.setTextColor(BLACK);
		display.setCursor(20, 0); display.println(" Scope ");
		if (mainMenu == 1)   display.setTextColor(WHITE, BLACK); else display.setTextColor(BLACK);
		display.setCursor(10, 8); display.println(" Generator ");
		if (mainMenu == 2)   display.setTextColor(WHITE, BLACK);
		else display.setTextColor(BLACK);
		display.setCursor(3, 18); display.println("DDS-generator");
		if (mainMenu == 3)   display.setTextColor(WHITE, BLACK); else display.setTextColor(BLACK);
		display.setCursor(13, 28); display.println(" Terminal ");
		display.setTextColor(BLACK);
		display.setCursor(12, 38); display.print("Vbat=");

		for (size_t i = 0; i < 30; i++)
		{
			adcBat = adcBat + analogRead(akb);
		}
		adcBat = adcBat / 30;
		
		display.print(adcBat*VCC / 1024);
		display.print("V");

		adcBat = 0;

		// перемещаемся по меню  
		if (flag_key - !digitalRead(key_up)) { mainMenu++; delay(100); }
		if (flag_key - !digitalRead(key_down)) { mainMenu--; delay(100); }
		if (mainMenu == 0xFF) mainMenu = 3;
		if (mainMenu > 3) mainMenu = 0;
		delay(50);
		display.display(); // выводим все на экран
	}
	// выбран режим работы...
	if (mainMenu == 0) FreqCount.begin(1000);
	if (mainMenu == 1) { InitTimersSafe(); bool success = SetPinFrequencySafe(led, freq); }
	if (mainMenu == 2) { InitTimersSafe(); bool success = SetPinFrequencySafe(led, 200000); }
	delay(200);
}

// читаем с АЦП данные и помещаем их в буфер...   
void ReadADC() {
	if (razv >= 6) ADCSRA = 0b11100010; //delitel 4
	if (razv == 5) ADCSRA = 0b11100011; //delitel 8
	if (razv == 4) ADCSRA = 0b11100100; //delitel 16
	if (razv == 3) ADCSRA = 0b11100101; //delitel 32
	if (razv == 2) ADCSRA = 0b11100110; //delitel 64
	if (razv < 2)  ADCSRA = 0b11100111; //delitel 128
	if (razv == 0) {
		for (int i = 0; i < 700; i++) {
			while ((ADCSRA & 0x10) == 0); // ждем готовность АЦП
			ADCSRA |= 0x10;               // запускаем следующее преобразование
			delayMicroseconds(500);     // зачем то делаем задержку
			mass[i] = ADCH;               // записываем данные в массив
		}
	}
	else { // (razv>0)
		for (int i = 0; i < 700; i++) {
			while ((ADCSRA & 0x10) == 0); // ждем готовность АЦП
			ADCSRA |= 0x10;               // запускаем следующее преобразование
			mass[i] = ADCH;               // записываем данные в массив
		}
	}
}

// безконечный цикл
void loop() {
	if (mainMenu == 0) { // если режим осциллоскопа, то..  
		if (opornoe) ADMUX = 0b01100011; // выбор внешнего опорного
		else ADMUX = 0b11100011; // выбор внутреннего опорного 1,1В
		delay(5);
		if (pause == 0) ReadADC();
		// определение точки синхронизации
		bool flagSINHRO = 0;
		bool flagSINHRnull = 0;
		for (int y = 1; y < 255; y++) {
			if (flagSINHRO == 0) { if (mass[y] < SinU) { flagSINHRnull = 1; } }
			if (flagSINHRO == 0) { if (flagSINHRnull == 1) { if (mass[y] > SinU) { flagSINHRO = 1; sinhMASS = y; } } }
		}
		// считаем максимальное значение сигнала (для вывода на экран)
		Vmax = 0;
		for (int y = 1; y < 255; y++) if (Vmax < mass[y]) Vmax = mass[y];
		// отрисовка графика 
		display.clearDisplay();
		if (pause) { // если в режиме паузы, то рисуем с прокруткой...
			display.drawLine(prokr / 8, 8, prokr / 8 + 6, 8, BLACK); //шкала прокрутки
			display.drawLine(prokr / 8, 9, prokr / 8 + 6, 9, BLACK); //шкала прокрутки
			x = 3;
			for (int y = prokr; y < prokr + 80; y++) { // рисуем форму сигнала
				if (razv < 7)  x++;
				if (razv == 7) x += 2;
				if (razv == 8) x += 3;
				display.drawLine(x, 47 - mass[y] / 7, x + 1, 47 - mass[y + 1] / 7, BLACK);
				display.drawLine(x + 1, 47 - mass[y] / 7 + 1, x + 2, 47 - mass[y + 1] / 7 + 1, BLACK);
			}
		}
		else {  // если паузы нет, то рисуем немного по другому))..
			display.fillCircle(80, 47 - SinU / 7, 2, BLACK); //рисуем уровень синхронизации    
			x = 3;
			for (int y = sinhMASS; y < sinhMASS + 80; y++) {  // рисуем форму сигнала
				if (razv < 7)  x++;
				if (razv == 7) x += 2;
				if (razv == 8) x += 3;
				display.drawLine(x, 47 - mass[y] / 7, x + 1, 47 - mass[y + 1] / 7, BLACK);
				display.drawLine(x + 1, 47 - mass[y] / 7 + 1, x + 2, 47 - mass[y + 1] / 7 + 1, BLACK);
			}
			sinhMASS = 0;
		}
		// отрисовка сетки
		for (byte i = 47; i > 5; i = i - 7) { display.drawPixel(0, i, BLACK); display.drawPixel(1, i, BLACK); display.drawPixel(2, i, BLACK); }
		for (byte i = 47; i > 5; i = i - 3) { display.drawPixel(21, i, BLACK); display.drawPixel(42, i, BLACK); display.drawPixel(63, i, BLACK); }
		for (byte i = 3; i < 84; i = i + 3) { display.drawPixel(i, 33, BLACK); display.drawPixel(i, 19, BLACK); }
		// отрисовка menu
		display.setCursor(0, 0);
		if (oscMenu == 0) {
			display.setTextColor(WHITE, BLACK);
			if (opornoe) display.print(VCC, 1); else display.print("1.1");
			display.setTextColor(BLACK);
			display.print(" "); display.print(razv);
			display.print(" P");
			if (flag_key - !digitalRead(key_down) || flag_key - !digitalRead(key_up)) { opornoe = !opornoe; }
		}
		if (oscMenu == 1) {
			if (opornoe) display.print(VCC, 1); else display.print("1.1");
			display.setTextColor(WHITE, BLACK);
			display.print(" "); display.print(razv);
			display.setTextColor(BLACK);
			display.print(" P");
			if (flag_key - !digitalRead(key_down)) { razv = razv - 1; if (razv == 255) razv = 0; }
			if (flag_key - !digitalRead(key_up)) { razv = razv + 1; if (razv == 9)   razv = 8; }
		}
		if (oscMenu == 2) {
			if (opornoe) display.print(VCC, 1); else display.print("1.1");
			display.print(" "); display.print(razv);
			display.setTextColor(WHITE, BLACK);
			display.print(" P");
			display.setTextColor(BLACK);
			pause = 1;
			if (flag_key - !digitalRead(key_down)) { prokr = prokr - 10; if (prokr < 0)   prokr = 0; }
			if (flag_key - !digitalRead(key_up)) { prokr = prokr + 10; if (prokr > 620) prokr = 620; }
		}
		if (oscMenu == 3) {
			prokr = 0;
			pause = 0;
			if (opornoe) display.print(VCC, 1); else display.print("1.1");
			display.print(" "); display.print(razv);
			display.print(" P");
			if (flag_key - !digitalRead(key_down)) { SinU = SinU - 20; if (SinU < 20)  SinU = 20; }
			if (flag_key - !digitalRead(key_up)) { SinU = SinU + 20; if (SinU > 230) SinU = 230; }
			display.fillCircle(80, 47 - SinU / 7, 5, BLACK);
			display.fillCircle(80, 47 - SinU / 7, 2, WHITE);
		}
		if (flag_key - !digitalRead(key_ok)) { oscMenu++; if (oscMenu == 4) { oscMenu = 0; pause = 0; } }//перебор меню
		if (FreqCount.available()) count = FreqCount.read(); //вывод частоты по готовности счетчика

															 // считаем частоту сигнала
		byte Frec1 = 0;
		long Frec = 0;
		bool flagFrec1 = 0;
		bool flagFrec2 = 0;
		bool flagFrec3 = 0;
		for (int y = 1; y < 255; y++) {
			if (flagFrec1 == 0 && mass[y] < SinU) flagFrec2 = 1;
			if (flagFrec1 == 0 && flagFrec2 == 1 && mass[y] > SinU) { flagFrec1 = 1; Frec1 = y; }
			if (flagFrec1 == 1 && mass[y] < SinU) flagFrec3 = 1;
			if (flagFrec3 == 1 && mass[y] > SinU) {
				if (razv >= 6) Frec = 1000000 / ((y - Frec1 - 1)*3.27);  //delitel 4
				if (razv == 5) Frec = 1000000 / ((y - Frec1)*3.27) / 2;  //delitel 8
				if (razv == 4) Frec = 1000000 / ((y - Frec1)*3.27) / 4;  //delitel 16
				if (razv == 3) Frec = 1000000 / ((y - Frec1)*3.27) / 8;  //delitel 32
				if (razv == 2) Frec = 1000000 / ((y - Frec1)*3.27) / 16; //delitel 64
				if (razv == 2) Frec = 1000000 / ((y - Frec1)*3.27) / 32; //delitel 128
				if (razv == 1) Frec = 1000000 / ((y - Frec1)*3.27) / 32; //delitel 128
				if (razv == 0) Frec = 1000000 / ((y - Frec1) * 500);     //delitel 128
				flagFrec1 = 0; flagFrec3 = 0;
			}
		}
		// отрисовка частоты сигнала
		if (opornoe == 1) {
			if ((Vmax*VCC / 255) > 2.5) countX = count*(overclock / 16.0);
			if ((Vmax*VCC / 255) < 2.5) countX = Frec*(overclock / 16.0);
		}
		else countX = Frec*(overclock / 16.0);
		if (countX < 1000) { display.print(" "); display.print(countX); display.print("Hz"); }
		if (countX > 1000) { float countXK = countX / 1000.0; display.print(countXK, 1); display.print("KHz"); }
		// отрисовка максимального напряжения сигнала
		display.setCursor(0, 40);
		if (opornoe) display.print(Vmax*VCC / 255, 1); else display.print(Vmax*1.1 / 255, 1);
		display.print("V");
		delay(200);
		display.display(); // вываливаем все на экран
	}
	if (mainMenu == 1) Generator();    // "выпадаем" в генератор 
	if (mainMenu == 2) DDSGenerator(); // "выпадаем" в DDS генератор
	if (mainMenu == 3) TTL();          // "выпадаем" в USART приемник
}

// РЕЖИМ ГЕНЕРАТОРА
void Generator() {
	display.clearDisplay();
	if (flag == 0) { //флаг выборов режима настройки ШИМ или Частоты
		if (flag_key - !digitalRead(key_down)) {
			freq -= stepFreq;
			if (freq < 0) freq = 0;
			bool success = SetPinFrequencySafe(led, freq);
			delay(100); // задержка от дребезга 
		}
		if (flag_key - !digitalRead(key_up)) {
			freq += stepFreq;
			bool success = SetPinFrequencySafe(led, freq);
			delay(100); // задержка от дребезга 
		}
	}
	if (flag == 1) { //флаг выборов режима настройки ШИМ или Частоты
		if (flag_key - !digitalRead(key_down)) {
			PWM = PWM - 1;
			if (PWM < 1) PWM = 100;
			delay(100); // задержка от дребезга
		}
		if (flag_key - !digitalRead(key_up)) {
			PWM = PWM + 1;
			if (PWM > 100) PWM = 1;
			delay(100);//защита от дребезга 
		}
	}
	if (flag_key - !digitalRead(key_ok)) {  //переключение разряда выбора частоты 
		delay(100);//защита от дребезга
		hag++;
		if (hag >= 5) hag = 0;
	}
	// выводим уровень ШИМ
	display.setTextSize(1);
	display.setCursor(21, 5);
	display.print("PWM=");
	display.print(PWM);
	display.print("%");
	// рисуем полосочки уровня ШИМ
	display.drawLine(41 - (41 * PWM / 100.0), 0, 42 + (41 * PWM / 100.0), 0, BLACK);
	display.drawLine(42 - (41 * PWM / 100.0), 1, 41 + (41 * PWM / 100.0), 1, BLACK);
	display.drawLine(43 - (41 * PWM / 100.0), 2, 40 + (41 * PWM / 100.0), 2, BLACK);
	display.drawLine(41 - (41 * PWM / 100.0), 16, 42 + (41 * PWM / 100.0), 16, BLACK);
	display.drawLine(42 - (41 * PWM / 100.0), 15, 41 + (41 * PWM / 100.0), 15, BLACK);
	display.drawLine(43 - (41 * PWM / 100.0), 14, 40 + (41 * PWM / 100.0), 14, BLACK);
	// готовимся для вывода частоты    
	display.setTextSize(2);
	long freqX = freq*(overclock / 16.0);
	// рисуем частоту
	if (freqX < 1000) {
		display.setCursor(18, 20); display.print(freqX);
		display.setTextSize(1);    display.print("Hz");
	}
	else {
		display.setCursor(4, 20);
		if (freqX < 10000)   display.print(freqX / 1000.0, 3);  else
			if (freqX < 100000)  display.print(freqX / 1000.0, 2);  else
				if (freqX < 1000000) display.print(freqX / 1000.0, 1);
		display.setTextSize(1); display.print("KHz");
	}
	// выводим строку статуса (режима)
	if (hag == 0) display.setCursor(11, 38);
	if (hag == 1) display.setCursor(15, 38);
	if (hag == 2) display.setCursor(12, 38);
	if (hag == 3) display.setCursor(8, 38);
	if (hag == 4) display.setCursor(2, 38);
	display.setTextSize(1);
	display.print(">>");
	display.setTextColor(WHITE, BLACK);
	display.print(" ");
	if (hag < 4) display.print("x");
	if (hag == 0) { // выбор множителя частоты        
		display.print(1 * (overclock / 16.0), 1);
		stepFreq = 1;
		flag = 0;
	}
	if (hag == 1) {//выбор множителя частоты
		display.print(10 * (overclock / 16.0), 0);
		stepFreq = 10;
	}
	if (hag == 2) {//выбор множителя частоты
		display.print(100 * (overclock / 16.0), 0);
		stepFreq = 100;
	}
	if (hag == 3) {//выбор множителя частоты
		display.print(1000 * (overclock / 16.0), 0);
		stepFreq = 1000;
	}
	if (hag == 4) {//выбор  PWM
		display.print("PWM ");
		display.print(PWM);
		display.print("%");
		flag = 1;
	}
	display.print(" ");
	display.setTextColor(BLACK);
	display.print("<<");
	// выставили ШИМ
	pwmWrite(led, PWM*2.55);
	delay(100);
	display.display();
}

// Рисуем меню DDS-генератора
void DDSGeneratorMenu() { // я думаю, что эта функция не нужна, разберусь позже.... [Electronik83]
	display.clearDisplay();
	if (ddsMenu == 0) display.setTextColor(WHITE, BLACK); else display.setTextColor(BLACK);
	display.setCursor(21, 0); display.println(" Sine ");
	if (ddsMenu == 1) display.setTextColor(WHITE, BLACK); else display.setTextColor(BLACK);
	display.setCursor(10, 10); display.println(" Triangle ");
	if (ddsMenu == 2) display.setTextColor(WHITE, BLACK); else display.setTextColor(BLACK);
	display.setCursor(24, 20); display.println(" Saw ");
	if (ddsMenu == 3) display.setTextColor(WHITE, BLACK); else display.setTextColor(BLACK);
	display.setCursor(16, 30); display.println(" invSaw ");
	display.setTextColor(BLACK);
	display.display();
}

// DDS генератор
void DDSGenerator() {
	if (ddsMenu == 0) {
		DDSGeneratorMenu();
		while (flag_key - digitalRead(key_up) && flag_key - digitalRead(key_down) && flag_key - digitalRead(key_ok)) {
			PWM = sinM[d];
			pwmWrite(dds, PWM);
			d++;
			if (d == 32) d = 0;
		}
		ddsMenu++;
		delay(200);
	}
	if (ddsMenu == 1) {
		DDSGeneratorMenu();
		while (flag_key - digitalRead(key_up) && flag_key - digitalRead(key_down) && flag_key - digitalRead(key_ok)) {
			PWM = trianglM[d];
			pwmWrite(dds, PWM);
			d++;
			if (d == 32) d = 0;
		}
		ddsMenu++;
		delay(200);
	}
	if (ddsMenu == 2) {
		DDSGeneratorMenu();
		while (flag_key - digitalRead(key_up) && flag_key - digitalRead(key_down) && flag_key - digitalRead(key_ok)) {
			PWM = pilaM[d];
			pwmWrite(dds, PWM);
			d++;
			if (d == 32) d = 0;
		}
		ddsMenu++;
		delay(200);
	}
	if (ddsMenu == 3) {
		DDSGeneratorMenu();
		while (flag_key - digitalRead(key_up) && flag_key - digitalRead(key_down) && flag_key - digitalRead(key_ok)) {
			PWM = RpilaM[d];
			pwmWrite(dds, PWM);
			d++;
			if (d == 32) d = 0;
		}
		ddsMenu = 0;
		delay(200);
	}
}

// UART приемник
void TTL() {
	display.clearDisplay();
	display.setTextColor(WHITE, BLACK);
	display.setCursor(0, 0); display.println("   Terminal   ");
	display.setTextColor(BLACK);
	display.setCursor(27, 10); display.println("Speed:");
	display.setTextColor(WHITE, BLACK);
	display.setCursor(10, 20); display.print("-");
	display.setTextColor(BLACK);
	display.println(" ");

	if(speedTTL < 10000) display.setCursor(30, 25);
	if (speedTTL >= 10000 && speedTTL <= 100000) display.setCursor(27, 25);
	if (speedTTL > 100000) display.setCursor(25, 25);

	display.print(speedTTL);
	display.println(" ");
	display.setTextColor(WHITE, BLACK);
	display.setCursor(69, 20);  display.println("+");
	display.setTextColor(BLACK);
	display.setCursor(12, 38); display.println("OK - start");
	if (flag_key - !digitalRead(key_up))   speedTTL = speedTTL + 1200;
	if (flag_key - !digitalRead(key_down)) speedTTL = speedTTL - 1200;
	if (speedTTL < 1200) speedTTL = 249600;
	if (speedTTL > 249600) speedTTL = 1200;
	if (flag_key - !digitalRead(key_ok)) {
		Serial.begin(speedTTL*(16 / overclock));
		display.clearDisplay();
		delay(100);
		display.display();
		int x = 0;
		int y = 0;
		while (1) {
			char incomingBytes;
			if (Serial.available() > 0) { // Если в буфере есть данные
				incomingBytes = Serial.read(); // Считывание байта в переменную incomeByte
				display.setCursor(x, y);
				display.print(incomingBytes); // Печать строки в буффер дисплея
				display.display(); x = x + 6;
				if (x == 84) { x = 0; y = y + 8; }
				if (y == 48) {
					x = 0; y = 0;
					display.clearDisplay();
					delay(100);
					display.display();
				}
			}
		}
	}
	delay(100);
	display.display();
}