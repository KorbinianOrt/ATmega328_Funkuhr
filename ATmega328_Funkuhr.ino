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


//in diesen Variablen wird die Uhrzeit gespeichert, bevor sie an den LCD übergeben wird
int Wochentag = 0;
int Stunde = 0;
int Minute = 0;
int Sekunde = 0;


//Variablen, um die Das DCF77 Signales zu interpretieren
//Zeitpunkt, an dem von HIGH auf LOW gewechselt wird
int ZeitpunktDown = 0;
//Zeitpunkt, an dem von LOW auf HIGH gewechselt wird
int ZeitpunktUp = 0;
//Dauer, wie lange das auf Signal LOW war
int DauerLow = 0;
//Dauer, wie lange das Signal auf HIGH war
int DauerHigh = 0;
//Es muss mitgezählt werden welcher Bit HIGH oder LOW war
bool DCF77Signal = false;
int DCF77Bitnummer = 0;
//DCF77Bitnummer initialisieren ist dafür da, dass erst ab dem tatsächlich ersten Bit des DCF77 mitgezählt wird. (damit das Signal nicht falsch interpretiert werden kann)
bool DCF77BitnummerZaehlen = false;
int DCF77Bitwert = 0;
bool DCF77FertigKalibriert = false;


//Knopf Flag wird in einem Interrupt gesetzt, der jedes mal ausgeführt wird, wenn ein Knopf gedrückt wird.
//Das soll dazu führen, dass wenig Latenz beim drücken der wert sich nur um 1 verändert und, dass das drücken der Knöpfe, wenn man die Uhrzeit manuel einstellt nicht viel Latenz hat
//Die ISR, in dem dieser Flag gesetzt wird muss noch eingerichtet werden (auf Steigende Flanke setzen)
volatile bool KnopfFlag = false;

//Wenn sich Pin2 ändert wird ein Flag gesetzt
volatile bool DCF77PinFlag = false;

//Wenn TCNT1 überläuft wird ein Flag gesetzt
volatile bool Timer1Flag = false;

//Wenn TCNT1 überläuft oder sich Pin2 ändert muss der Controller aus dem Schlaf geweckt werden
volatile bool SchlafenFlag = false;
//Schlafen ignoriere ich vorerst. Es wird dann eingebaut, wenn die Uhr praktisch funktioniert

//Wenn FunkuhrModus == 1, dann Funkuhr; Wenn FunkuhrModus == 0 autonome Uhr
bool FunkuhrModus = true;

//Wenn ManuelEinstellen == 1 befindet man sich im Modus, in dem man die Uhrzeit manuell einstellen kann
bool ManuelEinstellen = false;
bool StundeEinstellen = false;
bool MinuteEinstellen = false;
bool SekundeEinstellen = false;


//Knöpfe, die ich noch hinzufügen möchte:
//aktivieren/Beenden der Funkuhr; Wenn im Manuell einstellen: Wechsel zwischen Stunde, Minute, Sekunde
//Manuelles Einstellen starten/beenden
//++
//--



//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Alle Funktionen hier:



void UhrzeitAnLCDSenden (int StundeLCD, int MinuteLCD, int SekundeLCD) {

  //Damit die Uhrzeit am LCD korrekt angezeigt wird muss man 
  int SekundeE = SekundeLCD % 10;
  int SekundeZ = SekundeLCD / 10 % 10;
  int MinuteE = MinuteLCD % 10;
  int MinuteZ = MinuteLCD / 10 % 10;
  int StundeE = StundeLCD % 10;
  int StundeZ = StundeLCD / 10 % 10;
  
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
  
  //Hier noch Einfügen: Wochentag: Zahl->Buchstaben und das dann an LCD senden
  //lcd.setCursor(9, 0);
  
  
}
  
  
void SekundeVergangen () {
  if (Sekunde < 59) {
    Sekunde++;
  }
  else {
    Sekunde = 0;
    if (Minute < 59) {
      Minute++;
    }
    else {
      Sekunde = 0;
      Minute = 0;
      if (Stunde < 23) {
        Stunde++;
      }
    }
  }
}


int MinuteDCF77 = 0;
int StundeDCF77 = 0;
int WochentagDCF77 = 0;

//Manche bits haben immer den gleichen Wert. Wenn die nicht stimmen könnte man rückschließen, dass die Antenne kein gutes Signal empfängt
bool FehlerAnAntenne = false;
bool FehlerBitnummer20 = false;
bool FehlerBitnummer0 = false;

void DCF77Interpretation () {
  //In dieser Funktion könnte man switch cases anstelle von etlichen if statements verwenden?
  Serial.println("Ich interpretiere: ");
  if (DCF77BitnummerZaehlen == false) {
    Serial.println("Schwachsinn!");
  }
  if (DCF77BitnummerZaehlen == true){
  Serial.print("DCF77Bitnummer = ");
  Serial.println(DCF77Bitnummer);
  Serial.print("DCF77Bitwert = ");
  Serial.println(DCF77Bitwert);
  }

  
  //Bit 0 = 0 immer. "Start einer neuen Minute"
  if (DCF77Bitnummer == 0){
    if (DCF77Bitwert == 0){
      FehlerBitnummer0 = false;
    }
    if (DCF77Bitwert == 1){
      FehlerBitnummer0 = true;
    }
  }

  //Bit 1-14 Wetterinformationen und Informationen des Katastrophenschutzes
  //Bit 15 Rufbit???
  //Bit 16 == 1: Am Ende dieser Stunde wird MEZ/MESZ umgestellt
  //Bit 17/18: 01=MEZ 10=MESZ
  //Bit 19 == 1: Am Ende dieser Stunde wird eine Schaltsekunde eingefügt
  
  
  //Bit 20 = 1 immer. "Beginn der Zeitinformation"
  if (DCF77Bitnummer == 20){
    if (DCF77Bitwert == 0){
      FehlerBitnummer20 = true;
    }
    if (DCF77Bitwert == 1){
      FehlerBitnummer20 = false;
    }
  }
  
  
  //Minuten Bit 21 bis 27
  //Darf Werte von 0 bis 59 annehmen
  if (DCF77Bitnummer == 21 && DCF77Bitwert == 1) {
    MinuteDCF77 = 1;
  }
  if (DCF77Bitnummer == 21 && DCF77Bitwert == 0) {
    MinuteDCF77 = 0;
  }
  if (DCF77Bitnummer == 22 && DCF77Bitwert == 1) {
    MinuteDCF77 = MinuteDCF77 + 2;
  }
  if (DCF77Bitnummer == 23 && DCF77Bitwert == 1) {
    MinuteDCF77 = MinuteDCF77 + 4;
  }
  if (DCF77Bitnummer == 24 && DCF77Bitwert == 1) {
    MinuteDCF77 = MinuteDCF77 + 8;
  }
  if (DCF77Bitnummer == 25 && DCF77Bitwert == 1) {
    MinuteDCF77 = MinuteDCF77 + 10;
  }
  if (DCF77Bitnummer == 26 && DCF77Bitwert == 1) {
    MinuteDCF77 = MinuteDCF77 + 20;
  }
  if (DCF77Bitnummer == 27 && DCF77Bitwert == 1) {
    MinuteDCF77 = MinuteDCF77 + 40;
  }
  
  
  //Stunden Bit 29-35
  //Darf Werte von 0 bis 23 annehmen
  if (DCF77Bitnummer == 29 && DCF77Bitwert == 1) {
    StundeDCF77 = 1;
  }
  if (DCF77Bitnummer == 29 && DCF77Bitwert == 0) {
    StundeDCF77 = 0;
  }
  if (DCF77Bitnummer == 30 && DCF77Bitwert == 1) {
    StundeDCF77 = StundeDCF77 + 2;
  }
  if (DCF77Bitnummer == 31 && DCF77Bitwert == 1) {
    StundeDCF77 = StundeDCF77 + 4;
  }
  if (DCF77Bitnummer == 32 && DCF77Bitwert == 1) {
    StundeDCF77 = StundeDCF77 + 8;
  }
  if (DCF77Bitnummer == 33 && DCF77Bitwert == 1) {
    StundeDCF77 = StundeDCF77 + 10;
  }
  if (DCF77Bitnummer == 34 && DCF77Bitwert == 1) {
    StundeDCF77 = StundeDCF77 + 20;
  }
  

  //Wochentag Bit 42 bis 44
  //Darf Werte von 0 bis 6 annehmen
  if (DCF77Bitnummer == 42 && DCF77Bitwert == 1) {
    int WochentagDCF77 = 1;
  }
  if (DCF77Bitnummer == 43 && DCF77Bitwert == 1) {
    WochentagDCF77 = WochentagDCF77 + 2;
  }
  if (DCF77Bitnummer == 44 && DCF77Bitwert == 1) {
    WochentagDCF77 = WochentagDCF77 + 4;
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
  DCF77PinFlag = true;
  SchlafenFlag = true;
  //Serial.println("Es ist etwas Passiert!");
}


//Interrupt Service Routine, der ausgelöst wird, wenn TCNT1 einen Overflow-flag im TIFR1-Register hinterlässt
ISR(TIMER1_OVF_vect) {
  //TCNT1-counter muss in diesem Interrupt auf Startwert für 1000ms pro Durchlauf gesetzt werden!
  TCNT1 = 3036;

  Timer1Flag = true;
  SchlafenFlag = true;
}




void loop() {


  //Wenn die Uhr im Funkuhr Modus ist zähle ich unabhängig von der Antenne die Sekunden mit
  //TCNT1 wird 1 mal in der Minute mit dem Funksignal synchronisiert
  if (FunkuhrModus == true && Timer1Flag == true){
      Timer1Flag = 0;
      
      SekundeVergangen ();

      
      if (DCF77FertigKalibriert == true){
      UhrzeitAnLCDSenden (Stunde,Minute,Sekunde);
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
    if (DCF77Signal == true) {
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
    if (DCF77Signal == false) {
    DCF77Interpretation ();
    if (DCF77FertigKalibriert == false){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("warte auf Signal");
      }
    }


    //Soll dazu dienen, die Bitnummer korrekt mit zu zählen
    if(DCF77BitnummerZaehlen == true && DCF77Signal == true){
    DCF77Bitnummer++;
    }
    if (DCF77Bitnummer == 59){
      Serial.println("DCF77Bitnummer = 59!");
      DCF77Bitnummer = 0;
      DCF77FertigKalibriert = true;     
    }



    
    //Beginn einer neuen Minute via Funksignal
    if (DauerLow > 1500 && DCF77Signal == true) {
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

      //Einmal in der Minute werden die Werte aus DCF77Interpretieren auf Minute, Stunde, Wochentag übertragen
      //So kann man verhindern, dass Wochentag, Stunde oder Minute Werte annimmt, die eigentlich gar nicht möglich sind. (z.b. 32:74)
      
      //Minute überprüfen
      if (MinuteDCF77 < 60) {
        Minute = MinuteDCF77;
        Serial.println("Minuten < 60");
        Serial.println(Minute);
      }
      else {
      Serial.println("Minute > 60!!!!!!");
      }

      //Stunde überprüfen
      if (StundeDCF77 < 24){
        Stunde = StundeDCF77;
        Serial.println("Stunden < 60");
        Serial.println(Stunde);
      }
      else {
      Serial.println("Stunde > 24!!!!!!");
      }

      //Wochentag überprüfen
      if (WochentagDCF77 < 7){
        Wochentag = WochentagDCF77;
        Serial.println("Wochentag < 7");
        Serial.println(Wochentag);
      }
      else {
        Serial.println("Wochentag > Sonntag lēl");
      }
      
      //WochentagDCF77, StundeDCF77 und MinuteDCF77 werden hier wieder auf 0 gesetzt, damit sie innerhalb der nächsten Minute komplett neu interpretiert werden können.
      WochentagDCF77 = 0;
      StundeDCF77 = 0;
      MinuteDCF77 = 0;

    }
    
  }


  if (FunkuhrModus == false && Timer1Flag == true){
    Timer1Flag = 0;
    SekundeVergangen ();
    UhrzeitAnLCDSenden (Stunde,Minute,Sekunde);
    
  }

  //Wenn man die Uhrzeit an der Uhr manuell einstellen möchte
  if (FunkuhrModus == false && ManuelEinstellen == true){
    if (StundeEinstellen == 1 && KnopfFlag == 1) {
      UhrzeitAnLCDSenden (Stunde,Minute,Sekunde);
      lcd.setCursor(0, 1);
      lcd.print ("XX:--:--");
    }
    if (MinuteEinstellen == false && KnopfFlag == true) {
      UhrzeitAnLCDSenden (Stunde,Minute,Sekunde);
      lcd.setCursor(0, 1);
      lcd.print ("--:XX:--");
    }

    //sollte man auch noch Sekunde einstellen können?
    //Idee: wenn man in den Manuellen einstellen Modus geht: Sekunde = 0; ?
    
  }
  
}
