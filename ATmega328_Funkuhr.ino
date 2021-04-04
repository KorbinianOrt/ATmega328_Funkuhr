//Diese Uhr benutzt den internen 16 Bit Timer
//Es gibt eine Option, mit der man die Uhr manuell stellen kann

//Es gibt eine Option, mit der man die Uhr mit einem Funksignal stellen kann.
//siehe https://de.wikipedia.org/wiki/DCF77#Amplitudenumtastung


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <LiquidCrystal.h>
//an diesen Pins liegt der LCD:
LiquidCrystal lcd(13, 12, 4, 5, 6, 7);
//Uno13 - LCD4 - Atmega19
//Uno12 - LCD6 - Atmega18
//Uno4 - LCD11 - Atmega6
//Uno5 - LCD12 - Atmega11
//Uno6 - LCD13 - Atmega12
//Uno7 - LCD14 - Atmega13



//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Alle Variablen und Constanten hier:


//in diesen Variablen wird die Uhrzeit mitgezählt
volatile int Counter = 1;
volatile int Tag = 0;
volatile int Stunde = 0;
volatile int Minute = 0;
volatile int Sekunde = 0;
//Zweistellige Zahlen sollten in Einer und Zehner zerlegt werden, damit sie auf dem LCD korrekt angezeigt werden
//Immer, wenn Stunde, Minute oder Sekunde an LCD gesendet werde muss man sie in Zehner und Einer zerlegen
volatile int SekundeE = Sekunde % 10;
volatile int SekundeZ = Sekunde / 10 % 10;
volatile int MinuteE = Minute % 10;
volatile int MinuteZ = Minute / 10 % 10;
volatile int StundeE = Stunde % 10;
volatile int StundeZ = Stunde / 10 % 10;

//Variablen, um die Das DCF77 Signales zu interpretieren
//Zeitpunkt, an dem von HIGH auf LOW gewechselt wird
volatile int ZeitpunktDown = 0;
//Zeitpunkt, an dem von LOW auf HIGH gewechselt wird
volatile int ZeitpunktUp = 0;
//Dauer, wie lange das auf Signal LOW war
volatile int DauerLow = 0;
//Dauer, wie lange das Signal auf HIGH war
volatile int DauerHigh = 0;
//Es muss mitgezählt werden welcher Bit HIGH oder LOW war
volatile int DCF77Signal = 0;
volatile int DCF77Bitnummer = 0;
//DCF77Bitnummer initialisieren ist dafür da, dass erst ab dem tatsächlich ersten Bit des DCF77 mitgezählt wird. (damit das Signal nicht falsch interpretiert werden kann)
volatile int DCF77BitnummerZaehlen = 0;
volatile int DCF77Bitwert = 0;
volatile int DCF77FertigKalibriert = 0;

//Wenn sich Pin2 ändert wird ein Flag gesetzt
volatile int DCF77PinFlag = 0;

//Wenn TCNT1 überläuft wird ein Flag gesetzt
volatile int Timer1Flag = 0;

//Wenn TCNT1 überläuft oder sich Pin2 ändert muss der Controller aus dem Schlaf geweckt werden
volatile int SchlafenFlag = 0;
//Schlafen ignoriere ich vorerst. Es wird dann eingebaut, wenn die Uhr praktisch funktioniert

//Wenn FunkuhrModus == 1, dann Funkuhr; Wenn FunkuhrModus == 0 autonome Uhr
volatile int FunkuhrModus = 1;

//Wenn ManuelEinstellen == 1 befindet man sich im Modus, in dem man die Uhrzeit manuell einstellen kann
volatile int ManuelEinstellen = 0;
volatile int StundeEinstellen = 0;
volatile int MinuteEinstellen = 0;
volatile int SekundeEinstllen = 0;

//Knopf Flag wird in einem Interrupt gesetzt, der jedes mal ausgeführt wird, wenn ein Knopf gedrückt wird.
//Das soll dazu führen, dass wenig Latenz beim drücken der wert sich nur um 1 verändert und, dass das drücken der Knöpfe, wenn man die Uhrzeit manuel einstellt nicht viel Latenz hat
//Die ISR, in dem dieser Flag gesetzt wird muss noch eingerichtet werden (auf Steigende Flanke setzen)
volatile int KnopfFlag = 0;

//Knöpfe, die ich noch hinzufügen möchte:
//aktivieren/Beenden der Funkuhr; Wenn im Manuell einstellen: Wechsel zwischen Stunde, Minute, Sekunde
//Manuelles Einstellen starten/beenden
//++
//--



//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Alle Funktionen hier:



void UhrzeitAnLCDSenden () {

  
  SekundeE = Sekunde % 10;
  SekundeZ = Sekunde / 10 % 10;
  MinuteE = Minute % 10;
  MinuteZ = Minute / 10 % 10;
  StundeE = Stunde % 10;
  StundeZ = Stunde / 10 % 10;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(StundeZ);
  lcd.print(StundeE);
  lcd.print(":");
  lcd.print(MinuteZ);
  lcd.print(MinuteE);
  lcd.print(":");
  lcd.print(SekundeZ);
  lcd.print(SekundeE);
}


void SekundeVergangen () {
  if (Sekunde < 59) {
    Sekunde++;
    SekundeE = Sekunde % 10;
    SekundeZ = Sekunde / 10 % 10;
  }
  else {
    Sekunde = 0;
    if (Minute < 59) {
      Minute++;
      MinuteE = Minute % 10;
      MinuteZ = Minute / 10 % 10;
    }
    else {
      Sekunde = 0;
      Minute = 0;
      if (Stunde < 23) {
        Stunde++;
        StundeE = Stunde % 10;
        StundeZ = Stunde / 10 % 10;
      }
    }
  }
}


void DCF77Interpretation () {

  Serial.println("Ich interpretiere: ");
  if (DCF77BitnummerZaehlen == 0) {
    Serial.println("Schwachsinn!");
  }
  if (DCF77BitnummerZaehlen == 1){
  Serial.print("DCF77Bitnummer = ");
  Serial.println(DCF77Bitnummer);
  Serial.print("DCF77Bitwert = ");
  Serial.println(DCF77Bitwert);
  }
  

  if (DCF77Bitnummer == 21 && DCF77Bitwert == 1) {
    Minute = 1;
  }
  if (DCF77Bitnummer == 21 && DCF77Bitwert == 0) {
    Minute = 0;
  }
  if (DCF77Bitnummer == 22 && DCF77Bitwert == 1) {
    Minute = Minute + 2;
  }
  if (DCF77Bitnummer == 23 && DCF77Bitwert == 1) {
    Minute = Minute + 4;
  }
  if (DCF77Bitnummer == 24 && DCF77Bitwert == 1) {
    Minute = Minute + 8;
  }
  if (DCF77Bitnummer == 25 && DCF77Bitwert == 1) {
    Minute = Minute + 10;
  }
  if (DCF77Bitnummer == 26 && DCF77Bitwert == 1) {
    Minute = Minute + 20;
  }
  if (DCF77Bitnummer == 27 && DCF77Bitwert == 1) {
    Minute = Minute + 40;
  }

  //Stunden Bit 29-35
  if (DCF77Bitnummer == 29 && DCF77Bitwert == 1) {
    Stunde = 1;
  }
  if (DCF77Bitnummer == 29 && DCF77Bitwert == 0) {
    Stunde = 0;
  }
  if (DCF77Bitnummer == 30 && DCF77Bitwert == 1) {
    Stunde = Stunde + 2;
  }
  if (DCF77Bitnummer == 31 && DCF77Bitwert == 1) {
    Stunde = Stunde + 4;
  }
  if (DCF77Bitnummer == 32 && DCF77Bitwert == 1) {
    Stunde = Stunde + 8;
  }
  if (DCF77Bitnummer == 33 && DCF77Bitwert == 1) {
    Stunde = Stunde + 10;
  }
  if (DCF77Bitnummer == 34 && DCF77Bitwert == 1) {
    Stunde = Stunde + 20;
  }
}



//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


void setup() {

  Serial.begin(115200);
  Serial.println("hi");

  //handelt es sich um ein anliegendes HIGH oder LOW signal?
  pinMode(2, INPUT);

  //I-Bit wird gesetzt -> Interrupts sind global aktiviert (S. 15)
  sei();

  //Interrupt Pin = INT0 = Pin2 = PD2
  //Aktivierung von Pin2 als Interrup über die Bitmaske (s.55)
  EIMSK = 0b00000001;

  //Interrupt-Ereignis: Änderung von Pin2 (S.54)
  //ISC01=0 und ISC00=1
  EICRA = 0b00000001;


  lcd.begin(16, 2);
  lcd.print(" ^.^  - void");
  lcd.setCursor(0, 1);
  lcd.print("o_0  - setup");
  delay (1000);
  lcd.clear();


  /* Erklärung zum hier Verwendeten Timer:

     TCCR = Timer/Counter Control Register
     Diese 8 Bit Register werden verwendet für alle Timer und Counter Einstellungen


     Ich möchte in das 16 Bit TCNT1 Register zählen (16-bit Timer/Counter1) siehe s. 89

     Ich möchte "Normal Mode" (siehe s.100)
     Hier wird ins TCNT1 16 bit Reigster gezählt. Wenn es voll ist wird TOV1 = 1 und TCNT1 = 0 gesetzt.
     Man kann Startwerte für TCNT1 angeben.
     WGM13 = 0
     WGM12 = 0
     WGM11 = 0
     WGM10 = 0
     siehe Tabelle s. 108

     Taktfrequenz = 16Mhz
     256 prescaler: -> Frequenz wird reduziert auf
     16Mhz/256 = 62500 Hz
     so wird in TCNT1 nur noch mit 62500 Hz gezählt
     (ist ca alle 1.048576 Sek voll)
     CS12 = 1
     CS11 = 0
     CS10 = 0
     siehe Tabelle S. 110

     Wenn das 16bit Counter Register (TCNT1) voll ist soll ein Interrupt getriggert werden
     im "Timer/Counter1 Interrupt Mask Register" (TISMK1) muss das "Timer/Counter1 Overflow Interrupt Enable" bit (TOIE1) gesetzt werden
     siehe S.112
     Im "Timer/Counter1 Interrupt Flag Register" (TIFR1) wird das "Timer Overflow Vector1" (TOV1) Bit bei einem Overflow von TCNT1 jetzt automatisch gesetzt.
     TOV1 wird automatisch gelöscht, wenn der Overflow Interrupt Vector ausgeführt wurde.
     siehe S.113

     Damit der Timer in 1 - Sekunden abständen ein Interrupt setzt (und nicht in 1.048576 Sek. Abständen) muss man bei TCNT1 einen Startwert setzen
     TCNT1 = 65536 - ( (Fclk) / (Prescaler*Ftarget) )
     Dieser Startwert muss jedes mal neu gesetzt werden, wenn TCNT1 überläuft

     Alle diese Bits befinden sich in den Registern:
     TCCR1A
     TCCR1B
     TIMSK1
     (TIFR1)
  */
  //Diese Werte müssen die Register haben:

  TCCR1A = 0b00000000;
  TCCR1B = 0b00000100;
  TIMSK1 = 0b00000001;

  //TCNT1 Startwert:
  TCNT1 = 3036;

}


//Interrupt Service Routine, der ausgeführt wird, wenn sich Pin 2 ändert.
ISR(INT0_vect) {
  DCF77PinFlag = 1;
  SchlafenFlag = 1;
  //Serial.println("Es ist etwas Passiert!");
}


//Interrupt Service Routine, der ausgelöst wird, wenn TCNT1 einen Overflow-flag im TIFR1-Register hinterlässt
ISR(TIMER1_OVF_vect) {
  //TCNT1-counter muss in diesem Interrupt auf Startwert für 1000ms pro Durchlauf gesetzt werden!
  TCNT1 = 3036;

  Timer1Flag = 1;
  SchlafenFlag = 1;
}




void loop() {


  //Wenn die Uhr im Funkuhr Modus ist zähle ich unabhängig von der Antenne die Sekunden mit
  //TCNT1 wird 1 mal in der Minute mit dem Funksignal synchronisiert
  if (FunkuhrModus == 1 && Timer1Flag == 1){
      Timer1Flag = 0;
      
      if (Sekunde < 59) {
        Sekunde++;
      }
      else {
        Sekunde = 0;
      }
      SekundeE = Sekunde % 10;
      SekundeZ = Sekunde / 10 % 10;
      
      if (DCF77FertigKalibriert == 1){
      UhrzeitAnLCDSenden ();
      }
  }

  
  //Wenn die Uhr im FunkuhrModus ist, befindet sie sich in Diesem Loop
  //DCF77PinFlag wird im ISR  auf 1 gesetzt (MC geht in diesen Loop, wenn sich das Signal am Antennenpin ändert)
  if (DCF77PinFlag == 1 && FunkuhrModus == 1) {
    DCF77PinFlag = 0;
    DCF77Signal = digitalRead(2);
    Serial.print("Pin 2: ");
    Serial.println(DCF77Signal);

    //Pin2: Low->High
    //Soll nur dazu dienen, um DauerLow korrekt zu messen
    if (DCF77Signal == 1) {
      ZeitpunktUp = millis();
      Serial.print("ZeitpunktUp: ");
      Serial.println(ZeitpunktUp);
      DauerLow = ZeitpunktUp - ZeitpunktDown;
    }
    else {
      ZeitpunktDown = millis();
      Serial.print("ZeitpunktDown: ");
      Serial.println(ZeitpunktDown);
      DauerHigh = ZeitpunktDown - ZeitpunktUp;
    }

    //DCF77Bitwert wird hier auf 1 oder 0 gesetzt
    if (DauerHigh > 150){
    DCF77Bitwert = 1;
    }
    else {
    DCF77Bitwert = 0;
    }


    //DCF77Bitnummer wird mit DCF77Bitwert interpretiert, Uhrzeit wird ans LCD geschickt
    if (DCF77Signal == 0) {
    DCF77Interpretation ();
    if (DCF77FertigKalibriert == 0){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("warte auf Signal");
      }
    }


    //Soll dazu dienen, die Bitnummer korrekt mit zu zählen
    if(DCF77BitnummerZaehlen == 1 && DCF77Signal == 1){
    DCF77Bitnummer++;
    }
    if (DCF77Bitnummer == 59){
      Serial.println("DCF77Bitnummer = 60!");
      DCF77Bitnummer = 0;
      DCF77FertigKalibriert = 1;     
    }



    
    //Beginn einer neuen Minute via Funksignal
    if (DauerLow > 1500 && DCF77Signal == 1) {
      Serial.println ("----- Beginn neuer Minute hier -----");
      Serial.println ("Es ist genau so viel Uhr!!!!");
      Serial.print(Stunde);
      Serial.print(" : ");
      Serial.println(Minute);
      DCF77BitnummerZaehlen = 1;
      DCF77Bitnummer = 0;
      Serial.println("DCF77Bitnummer = 0");
      
      Sekunde = 0;
      TCNT1 = 3036;
      
      MinuteE = Minute % 10;
      MinuteZ = Minute / 10 % 10;
      StundeE = Stunde % 10;
      StundeZ = Stunde / 10 % 10;
    }
  }


  if (FunkuhrModus == 0 && Timer1Flag == 1){
    Timer1Flag = 0;
    SekundeVergangen ();
    UhrzeitAnLCDSenden ();
    
  }

  //Wenn man die Uhrzeit an der Uhr manuell einstellen möchte
  if (FunkuhrModus == 0 && ManuelEinstellen == 1){
    if (StundeEinstellen == 1 && KnopfFlag == 1) {
      UhrzeitAnLCDSenden ();
      lcd.setCursor(0, 1);
      lcd.print ("XX:--:--");
    }
    if (MinuteEinstellen == 1 && KnopfFlag == 1) {
      UhrzeitAnLCDSenden ();
      lcd.setCursor(0, 1);
      lcd.print ("--:XX:--");
    }
  }
  
}
