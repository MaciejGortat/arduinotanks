#include <ISAOLED.h>
#include <ISADefinitions.h>
#include <ISALiquidCrystal.h>
#include <ISAButtons.h>
#include "mapa.h"
ISALiquidCrystal lcd;
ISAOLED oled;
ISAButtons button;

#define SYSRESETREQ    (1<<2)
#define VECTKEY        (0x05fa0000UL)
#define VECTKEY_MASK   (0x0000ffffUL)
#define AIRCR          (*(uint32_t*)0xe000ed0cUL)
#define REQUEST_EXTERNAL_RESET (AIRCR=(AIRCR&VECTKEY_MASK)|VECTKEY|SYSRESETREQ);

enum kierunek{
	GORA, DOL, LEWO, PRAWO, NONE
};
struct pixel{
	int x;
	int y;
};

struct pixel* pixele;

int zapiszPixele(){
	int temp = 0, j = 0;
	int miejsce = 0;
	int iks;
	for(int i = 128; i < (63 * 128); ++i){
		if(header_data[i] == 1 && ((i % 128) > 0 && (i % 128) < 127)){
			++miejsce;
		}
	}
	lcd.setCursor(0, 1);
	lcd.print(miejsce);
	pixele = (struct pixel*)malloc(sizeof(pixel) * miejsce);
	for(int i = 128; i < (63 * 128); ++i){
		if(header_data[i] == 1){
			iks = i % 128;
			if(iks > 0 && iks < 127){
				pixele[j].x = iks;
				pixele[j].y = temp;
				++j;
			}
		}
		if(i % 128 == 0 && i > 0) ++temp;
	}
	return miejsce;
	
}

int iloscPixeli;

struct gracz{
	short zycie;
	int punkty;
	kierunek ostatnio;
	bool ochrona;
	struct{
		short x;
		short y;
	}pos;
	private:
		int id;
	public:
		gracz(short startx, short starty, kierunek poczatek, int n){
			pos.x = startx;
			pos.y = starty;
			ostatnio = poczatek;
			id = n;
			zycie = 3;
			ochrona = true;
		}
	int getID(){
		return id;
	}
	void dajZycieNaLCD(){
		lcd.print("Zycie P");
		lcd.print(id);
		lcd.print("=");
		lcd.print(zycie);
	}
	short dajZycie(){
		return zycie;
	}
};

struct gracz gracz1(7, 7, PRAWO, 1);
struct gracz gracz2(120, 7, LEWO, 2);

//----------------------OBSLUGA POCISKOW-----------------------------------
struct pocisk{
	short x;
	short y;
	kierunek kp;
	int id_wlasciciela;
	unsigned int nr_pocisku = 0;
	bool aktywny;
};

struct pociski{
	pocisk tablica_pociskow[128];
	//short liczba_pociskow = 128;
	unsigned int licznik_p = 0;
	int aktualne_pociski = 0;
	void dodaj_pocisk(const pocisk& p){
		tablica_pociskow[(licznik_p % 128)] = p;
		++licznik_p;
		++aktualne_pociski;
	}
	void usun_pocisk(const pocisk& p){
		for(int i = 0; i < 128; ++i){
			if(p.nr_pocisku == tablica_pociskow[i].nr_pocisku){
				tablica_pociskow[i].aktywny = false; --aktualne_pociski;
				return;
			}
		}
	}
	//usuwa jesli trafi w sciane
	void rysuj_przesun_pociski(){
		bool pomin = false;
		for(int i = 0; i < 128; ++i){
			if(tablica_pociskow[i].aktywny == true){
				
				for(int j = 0; j < iloscPixeli; ++j){
					if(tablica_pociskow[i].x == pixele[j].x && tablica_pociskow[i].y == pixele[j].y){
						tablica_pociskow[i].aktywny = false; --aktualne_pociski;
						pixele[j] = pixele[iloscPixeli - 1];
						--iloscPixeli;
						pomin = true;
						break;
					}
				}
				if(pomin){
					continue;
				}
				for(int j = 0; j < 128; ++j){
					if(j != i){
						if(tablica_pociskow[j].aktywny == true){
							if(tablica_pociskow[i].x == tablica_pociskow[j].x && tablica_pociskow[i].y == tablica_pociskow[j].y){
								tablica_pociskow[i].aktywny = false; --aktualne_pociski;
								tablica_pociskow[j].aktywny = false; --aktualne_pociski;
								pomin = true;
							}
						}
					}
				}
				oled.setPixel(tablica_pociskow[i].x, tablica_pociskow[i].y, true);
				if(pomin){
					continue;
				}	
				if(tablica_pociskow[i].kp == GORA){
					--(tablica_pociskow[i].y);
					if(tablica_pociskow[i].y <= 0) {
						tablica_pociskow[i].aktywny = false; --aktualne_pociski;
					}
				}
				if(tablica_pociskow[i].kp == DOL){
					++(tablica_pociskow[i].y);
					if(tablica_pociskow[i].y >= 128) {
						tablica_pociskow[i].aktywny = false; --aktualne_pociski;
					}
				}
				if(tablica_pociskow[i].kp == LEWO){
					--(tablica_pociskow[i].x);
					if(tablica_pociskow[i].x <= 0) {
						tablica_pociskow[i].aktywny = false; --aktualne_pociski;
					}
				}
				if(tablica_pociskow[i].kp == PRAWO){
					++(tablica_pociskow[i].x);
					if(tablica_pociskow[i].x >= 128) {
						tablica_pociskow[i].aktywny = false; --aktualne_pociski;				}
				}
			}
		}
	}
	void dajLicznikPociskowNaLCD(){
		lcd.print("Razem ");
		lcd.print(licznik_p);
	}
	void dajAktualnePociskiNaLCD(){
		lcd.print("Teraz ");
		lcd.print(aktualne_pociski);
		//aktualne_pociski = 0;
	}
};

struct pociski przechowujePociski;

bool kolizjaPociskCzolg(struct pociski* pepek, struct gracz* czolgasek){
	struct p{
		short x;
		short y;
	};
	struct p ppp;
	struct p cpp;
	for(int i = 0; i < 128; ++i){
		ppp.x = pepek->tablica_pociskow[i].x;
		ppp.y = pepek->tablica_pociskow[i].y;
		cpp.x = czolgasek->pos.x;
		cpp.y = czolgasek->pos.y;

		//-----------------POCZATEK WARUNKU POCISK-CZOLG---------------------------
		//piksele leca od lewej do prawej, od gory do dolu, z pominieciem srodkowego
		if(pepek->tablica_pociskow[i].aktywny == true){
			if(
				(
					(czolgasek->ostatnio == GORA)					&& (
					(ppp.x == cpp.x + 1 && ppp.y == cpp.y)			||
					(ppp.x == cpp.x 	&& ppp.y == cpp.y + 1)		||
					(ppp.x == cpp.x + 2 && ppp.y == cpp.y + 1)		||
					(ppp.x == cpp.x 	&& ppp.y == cpp.y + 2)		||
					(ppp.x == cpp.x + 1 && ppp.y == cpp.y + 2)		||
					(ppp.x == cpp.x + 2 && ppp.y == cpp.y + 2)		)
				) || (
					(czolgasek->ostatnio == DOL)					&& (
					(ppp.x == cpp.x 	&& ppp.y == cpp.y)			||
					(ppp.x == cpp.x + 1 && ppp.y == cpp.y)			||
					(ppp.x == cpp.x + 2 && ppp.y == cpp.y)			||
					(ppp.x == cpp.x 	&& ppp.y == cpp.y + 1)		||
					(ppp.x == cpp.x + 2 && ppp.y == cpp.y + 1)		||
					(ppp.x == cpp.x + 1 && ppp.y == cpp.y + 2)		)
				) || (
					(czolgasek->ostatnio == LEWO)					&& (
					(ppp.x == cpp.x + 1 && ppp.y == cpp.y)			||
					(ppp.x == cpp.x + 2 && ppp.y == cpp.y)			||
					(ppp.x == cpp.x 	&& ppp.y == cpp.y + 1)		||
					(ppp.x == cpp.x + 2 && ppp.y == cpp.y + 1)		||
					(ppp.x == cpp.x + 1 && ppp.y == cpp.y + 2)		||
					(ppp.x == cpp.x + 2 && ppp.y == cpp.y + 2)		)
				) || (
					(czolgasek->ostatnio == PRAWO)					&& (
					(ppp.x == cpp.x 	&& ppp.y == cpp.y)			||
					(ppp.x == cpp.x + 1 && ppp.y == cpp.y)			||
					(ppp.x == cpp.x 	&& ppp.y == cpp.y + 1)		||
					(ppp.x == cpp.x + 2 && ppp.y == cpp.y + 1)		||
					(ppp.x == cpp.x 	&& ppp.y == cpp.y + 2)		||
					(ppp.x == cpp.x + 1 && ppp.y == cpp.y + 2)		))){
				//---------KONIEC WARUNKU POCISK-CZOLG-------------------------------------
				pepek->usun_pocisk(pepek->tablica_pociskow[i]);
				if(czolgasek->ochrona == false){
					czolgasek->zycie--;
					if(czolgasek->getID() == 1){
						czolgasek->pos.x = 7;
						czolgasek->pos.y = 7;
					}
					if(czolgasek->getID() == 2){
						czolgasek->pos.x = 120;
						czolgasek->pos.y = 7;
					}
					czolgasek->ochrona = true;
				}	
			}
		}
	}
	if(czolgasek->zycie == 0) return true; return false;
}

void wystrzelPocisk(struct gracz* czolg, unsigned int n){
	kierunek lufa;
	lufa = czolg->ostatnio;
	pocisk poc;
	poc.x = czolg->pos.x;
	poc.y = czolg->pos.y;
	if(lufa == GORA){
		poc.x = (czolg->pos.x) + 1;
		poc.y = (czolg->pos.y) - 1;
	}
	if(lufa == DOL){
		poc.x = (czolg->pos.x) + 1;
		poc.y = (czolg->pos.y) + 3;
	}
	if(lufa == LEWO){
		poc.x = (czolg->pos.x) - 1;
		poc.y = (czolg->pos.y) + 1;
	}
	if(lufa == PRAWO){
		poc.x = (czolg->pos.x) + 3;
		poc.y = (czolg->pos.y) + 1;
	}
	poc.kp = czolg->ostatnio;
	poc.id_wlasciciela = czolg->getID();
	poc.nr_pocisku = n;
	poc.aktywny = true;
	przechowujePociski.dodaj_pocisk(poc);
}

//-------------------------------------------------------------------------

//----------------------OBLSUGA PRZYCISKOW---------------------------------
bool pd2 = 0;
bool old_pd2 = 0;

bool czytajButton1(){
	return button.buttonPressed(15);
}

bool czytajButton2(){
	pd2 = !digitalRead(KEY_RIGHT);
	if (pd2 && !old_pd2){
		old_pd2 = pd2;
		return true;
	}
	old_pd2 = pd2;
	return false;
}

//-------------------------------------------------------------------------

//---------------------------OBSLUGA JOYPADOW------------------------------
struct joyXY{
	short x;
	short y;
	private:
		short joyx;
		short joyy;
	public:
		joyXY(short pinx, short piny){
			joyx = pinx;
			joyy = piny;
		}
	short getJoyX(){
		return joyx;
	}
	short getJoyY(){
		return joyy;
	}

};
struct joyXY gracz1joy(JOY1X, JOY1Y);
struct joyXY gracz2joy(JOY2X, JOY2Y);

void czytajJoy(struct joyXY* z){
	if(z->getJoyX() == JOY1X){
		z->x = 1023 - analogRead(z->getJoyX());
	}else{
		z->x = analogRead(z->getJoyX());
	}
	z->y = analogRead(z->getJoyY());
}
//-------------------------------------------------------------------------

//-------------------OBSLUGA CZOLGOW---------------------------------------


kierunek sprawdzKolizjeGracza(struct gracz* a, struct gracz* b){
	struct p{
		short x;
		short y;
	};
	struct p pos1;
	struct p pos2;
	pos1.x = a->pos.x;
	pos2.x = b->pos.x;
	pos1.y = a->pos.y;
	pos2.y = b->pos.y;
	short odlegloscY = abs(pos2.y - pos1.y);
	short odlegloscX = abs(pos2.x - pos1.x);
	for(int i = 0; i < iloscPixeli; ++i){
		if(a->ostatnio == GORA){
			if(pixele[i].y == pos1.y - 1 && (pixele[i].x == pos1.x || pixele[i].x == pos1.x + 1 || pixele[i].x == pos1.x + 2)){
				return GORA;
			}
		}
		if(a->ostatnio == DOL){
			if(pixele[i].y == pos1.y + 3 && (pixele[i].x == pos1.x || pixele[i].x == pos1.x + 1 || pixele[i].x == pos1.x + 2)){
				return DOL;
			}
		}
		if(a->ostatnio == PRAWO){
			if(pixele[i].x == pos1.x + 3 && (pixele[i].y == pos1.y || pixele[i].y == pos1.y + 1 || pixele[i].y == pos1.y + 2)){
				return PRAWO;
			}
		}
		if(a->ostatnio == LEWO){
			if(pixele[i].x == pos1.x - 1 && (pixele[i].y == pos1.y || pixele[i].y == pos1.y + 1 || pixele[i].y == pos1.y + 2)){
				return LEWO;
			}
		}
	}
	if(pos1.y - 3 == pos2.y && odlegloscX < 3) return GORA;
	if(pos1.y + 3 == pos2.y && odlegloscX < 3) return DOL;
	if(pos1.x + 3 == pos2.x && odlegloscY < 3) return PRAWO;
	if(pos1.x - 3 == pos2.x && odlegloscY < 3) return LEWO;
	return NONE;
}

void checkAndRun(struct joyXY* z, struct gracz* g1, struct gracz* g2, int jjj){
	czytajJoy(z);
	kierunek k = NONE;
	if(z->x > 800){
		g1->ochrona = false;
		g1->ostatnio = PRAWO;
		k = sprawdzKolizjeGracza(g1, g2);
		if(g1->pos.x == 124){
			rysujGraczPrawo(g1->pos.x, g1->pos.y);
			g1->ostatnio = PRAWO;
			return;
		}
		if(k != PRAWO && jjj % 2 == 0) 
			++(g1->pos.x);
		rysujGraczPrawo(g1->pos.x, g1->pos.y);
		return;
	}
	else if(z->x < 200){
		g1->ochrona = false;
		g1->ostatnio = LEWO;
		k = sprawdzKolizjeGracza(g1, g2);
		if(g1->pos.x == 1){
			rysujGraczLewo(g1->pos.x, g1->pos.y);
			g1->ostatnio = LEWO;
			return;
		}
		if(k != LEWO && jjj % 2 == 0) 
			--(g1->pos.x);
		rysujGraczLewo(g1->pos.x, g1->pos.y);
		return;
	}
	else if(z->y > 800){
		g1->ochrona = false;
		g1->ostatnio = GORA;
		k = sprawdzKolizjeGracza(g1, g2);
		if(g1->pos.y == 1){
			rysujGraczGora(g1->pos.x, g1->pos.y);
			g1->ostatnio = GORA;
			return;
		}
		if(k != GORA && jjj % 2 == 0) 
			--(g1->pos.y);
		rysujGraczGora(g1->pos.x, g1->pos.y);
		
		return;
	}
	else if(z->y < 200){
		g1->ochrona = false;
		g1->ostatnio = DOL;
		k = sprawdzKolizjeGracza(g1, g2);
		if(g1->pos.y == 60){
			rysujGraczDol(g1->pos.x, g1->pos.y);
			g1->ostatnio = DOL;
			return;
		}
		if(k != DOL && jjj % 2 == 0) 
			++(g1->pos.y);
		rysujGraczDol(g1->pos.x, g1->pos.y);
		return;
	}
	if(g1->ostatnio == LEWO) {rysujGraczLewo(g1->pos.x, g1->pos.y); return;}
	if(g1->ostatnio == PRAWO) {rysujGraczPrawo(g1->pos.x, g1->pos.y); return;}
	if(g1->ostatnio == GORA) {rysujGraczGora(g1->pos.x, g1->pos.y); return;}
	if(g1->ostatnio == DOL) {rysujGraczDol(g1->pos.x, g1->pos.y); return;}
}

void rysujGraczGora(int x, int y){
	//oled.setPixel(x, y, true);
	oled.setPixel((x + 1), y, true);
	//oled.setPixel((x + 2), y, true);
	oled.setPixel(x, (y + 1), true);
	oled.setPixel((x + 1), (y + 1), true);
	oled.setPixel((x + 2), (y + 1), true);
	oled.setPixel(x, (y + 2), true);
	oled.setPixel((x + 1), (y + 2), true);
	oled.setPixel((x + 2), (y + 2), true);
}
void rysujGraczDol(int x, int y){
	oled.setPixel(x, y, true);
	oled.setPixel((x + 1), y, true);
	oled.setPixel((x + 2), y, true);
	oled.setPixel(x, (y + 1), true);
	oled.setPixel((x + 1), (y + 1), true);
	oled.setPixel((x + 2), (y + 1), true);
	//oled.setPixel(x, (y + 2), true);
	oled.setPixel((x + 1), (y + 2), true);
	//oled.setPixel((x + 2), (y + 2), true);
}
void rysujGraczPrawo(int x, int y){
	oled.setPixel(x, y, true);
	oled.setPixel((x + 1), y, true);
	//oled.setPixel((x + 2), y, true);
	oled.setPixel(x, (y + 1), true);
	oled.setPixel((x + 1), (y + 1), true);
	oled.setPixel((x + 2), (y + 1), true);
	oled.setPixel(x, (y + 2), true);
	oled.setPixel((x + 1), (y + 2), true);
	//oled.setPixel((x + 2), (y + 2), true);
}
void rysujGraczLewo(int x, int y){
	//oled.setPixel(x, y, true);
	oled.setPixel((x + 1), y, true);
	oled.setPixel((x + 2), y, true);
	oled.setPixel(x, (y + 1), true);
	oled.setPixel((x + 1), (y + 1), true);
	oled.setPixel((x + 2), (y + 1), true);
	//oled.setPixel(x, (y + 2), true);
	oled.setPixel((x + 1), (y + 2), true);
	oled.setPixel((x + 2), (y + 2), true);
}

//-------------------------------------------------------------------------

void rysujMape(int jj){
	oled.clear(false);
	for(int i = 0; i < iloscPixeli; ++i){
		oled.setPixel(pixele[i].x , pixele[i].y, true);
	}
	oled.writeRect(0, 0, 128, 64, false);
	przechowujePociski.rysuj_przesun_pociski();
	if(kolizjaPociskCzolg(&przechowujePociski, &gracz1)){
		lcd.print("Wygral gracz 2");
		delay(3000);
		REQUEST_EXTERNAL_RESET;
	}
	if(kolizjaPociskCzolg(&przechowujePociski, &gracz2)){
		lcd.print("Wygral gracz 1");
		delay(3000);
		REQUEST_EXTERNAL_RESET;
	}
	
	//lcd.print(gracz1.dajZycie());
	//lcd.setCursor(0, 1);
	//lcd.print(gracz2.dajZycie());
	for(int i = 0; i < gracz1.dajZycie(); ++i){
			digitalWrite(LEDS[i], HIGH);
	}
	for(int i = 7; i > 7 - gracz2.dajZycie(); --i){
			digitalWrite(LEDS[i], HIGH);
	}
	
	bool b1 = false, b2 = false;
	b1 = czytajButton1();
	if(b1) wystrzelPocisk(&gracz1, przechowujePociski.licznik_p);
	b2 = czytajButton2();
	if(b2) wystrzelPocisk(&gracz2, przechowujePociski.licznik_p);
	//przechowujePociski.rysuj_przesun_pociski();
	checkAndRun(&gracz1joy, &gracz1, &gracz2, jj);
	checkAndRun(&gracz2joy, &gracz2, &gracz1, jj);
 	oled.renderAll();	
}

void setup(){
	lcd.begin();
	oled.begin();
	iloscPixeli = zapiszPixele();
	for(int i = 0; i < 8; ++i){
		pinMode(LEDS[i], OUTPUT);
	}
	pinMode(10, OUTPUT);
	digitalWrite(10, HIGH);
	pinMode(BUZZER, OUTPUT);
	digitalWrite(BUZZER, LOW);
	
	pinMode(KEY_RIGHT, INPUT);
	button.init();
}

void loop(){
	static int j = 0;
	j++;
	rysujMape(j);
	//przechowujePociski.dajAktualnePociskiNaLCD();
	//lcd.setCursor(0, 1);
	//przechowujePociski.dajLicznikPociskowNaLCD();
	delay(30);
	for(int i = 0; i < 8; ++i){
		digitalWrite(LEDS[i], LOW);
	}
	lcd.clear();
}
