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

//DCF77 Werte werden alle an ihrer eigenen Stelle an einem Array gespeichert.
bool DCF77Array [60] = {0};
void DCF77ArrayMitschreiben () {
  DCF77Array[DCF77Bitnummer] = DCF77Bitwert;
  
  Serial.print(DCF77Bitnummer);
  Serial.print(":");
  Serial.println(DCF77Bitwert);  
}


int MinuteDCF77 = 0;
int StundeDCF77 = 0;
int WochentagDCF77 = 0;


//Manche bits haben immer den gleichen Wert. Wenn die nicht stimmen könnte man rückschließen, dass die Antenne kein gutes Signal empfängt
bool FehlerAnAntenne = false;
bool FehlerBitnummer20 = false;
bool FehlerBitnummer0 = false;
int BitsMinute = 0;
int BitsStunde = 0;
int BitsDatum = 0;
bool FehlerParitaetStunde = false;
bool FehlerParitaetMinute = false;
bool FehlerParitaetDatum = false;
bool FehlerDCF77HighDauer = false;
bool FehlerDCF77LowDauer = false;

//Aus dem DCF77Array werden jetzt alle Werte interpretiert und auf potentielle Fehler überprüft
//Das muss 1 mal pro Minute gemacht werden
void DCF77ArrayInterpretieren () {
  WochentagDCF77 = DCF77Array[42] * 1 + DCF77Array[43] * 2 + DCF77Array[44] * 4;
  StundeDCF77 = DCF77Array[29] * 1 + DCF77Array[30] * 2 + DCF77Array[31] * 4 + DCF77Array[32] * 8 + DCF77Array[33] * 10 + DCF77Array[34] * 20;
  MinuteDCF77 = DCF77Array[21] * 1 + DCF77Array[22] * 2 + DCF77Array[23] * 4 + DCF77Array[24] * 8 + DCF77Array[25] * 10 + DCF77Array[26] * 20 + DCF77Array[27] * 40;

  //Prüfung Parität Datum
  BitsDatum = DCF77Array[36] + DCF77Array[37] + DCF77Array[38] + DCF77Array[39] + DCF77Array[40] + DCF77Array[41] + DCF77Array[42] + DCF77Array[43] + DCF77Array[44] + DCF77Array[45] + DCF77Array[46] + DCF77Array[47] + DCF77Array[48] + DCF77Array[49] + DCF77Array[50] + DCF77Array[51] + DCF77Array[52] + DCF77Array[53] + DCF77Array[54] + DCF77Array[55] + DCF77Array[56] + DCF77Array[57] + DCF77Array[58] ;
  if (BitsDatum % 2) {
    FehlerParitaetDatum = true;
  }
  else {
    FehlerParitaetDatum = false;
  }
  //Prüfung Parität Stunde
  BitsStunde = DCF77Array[29] + DCF77Array[30] + DCF77Array[31] + DCF77Array[32] + DCF77Array[33] + DCF77Array[34] + DCF77Array[35];
  if (BitsStunde % 2) {
    FehlerParitaetStunde = true;
  }
  else {
    FehlerParitaetStunde = false;
  }
  //Prüfung Parität Minute
  BitsMinute = DCF77Array[21] + DCF77Array[22] + DCF77Array[23] + DCF77Array[24] + DCF77Array[25] + DCF77Array[26] + DCF77Array[27] + DCF77Array[28];
  if (BitsMinute % 2) {
    FehlerParitaetMinute = true;
  }
  else {
    FehlerParitaetMinute = false;
  }
  FehlerBitnummer20 = !DCF77Array[20];
  FehlerBitnummer0 = DCF77Array[0];
  FehlerAnAntenne = FehlerBitnummer0 || FehlerBitnummer20 || FehlerParitaetStunde || FehlerParitaetStunde || FehlerParitaetDatum || FehlerDCF77HighDauer || FehlerDCF77LowDauer;
}


void UhrzeitAnLCDSenden (int StundeLCD, int MinuteLCD, int SekundeLCD) {

  if (DCF77FertigKalibriert == false) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("warte auf Signal");
    //Serial.println("Warte auf Signal");
  }
  else {
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
}


void SekundeVergangen () {
  if (Sekunde < 59) {
    Sekunde++;
  }
  else {
    Sekunde = 0;
    Serial.println("Minute vergangen");
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

  //16Bit Timer Register TCNT1: (für Sekunden)
  /* Erklärung zum 16 Bit Timer:
     Ich möchte in das 16 Bit TCNT1 Register zählen, um so 1 mal pro Sekunde einen interrupt triggern zu können (16-bit Timer/Counter1) siehe s. 89

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
  //Diese Werte müssen die Register haben: (siehe Seite 108 ff)
  TCCR1A = 0b00000000;
  TCCR1B = 0b00000100;
  TIMSK1 = 0b00000001;
  //TCNT1 Startwert:
  TCNT1 = 3036;

  //8Bit Timer Register TCNT2: (für Zeit zischen Änderungen am Antennenpin)
  /*Erklärung zum 8 Bit Timer:
    Ich möchte Normal Mode verwenden (siehe Seite 116ff)
    WGM22 = 0
    WGM21 = 0
    WGM20 = 0
    "In normal operation the TimerOverflowFlag (TOV2) will be set in the same timer clock cycle as the TCNT2 becomes 0"

    Prescaler: (siehe Seite 131ff)
    mit einem 1024 prescaler hat das 8 bit Register 61 mal in der Sekunde einen Overflow
    CS22 = 1
    CS21 = 1
    CS20 = 1
    6.1 mal für 0.1 Sek (logische 0)
    12.2 mal für 0.2 Sek (logische 1)
    109.8 oder 115.9 mal für Pause zwischen BitNummer 58 und 0

    Ich brauche eine Variable, in der +1 gerechnet wird, jedes mal, wenn TCNT2 einen Overflow hat

    Wenn das 8bit Counter Register (TCNT2) voll ist soll ein Interrupt getriggert werden
    im "Timer/Counter2 Interrupt Mask Register" (TISMK2) muss das "Timer/Counter2 Overflow Interrupt Enable" bit (TOIE2) gesetzt werden
    siehe S.88
    Im "Timer/Counter2 Interrupt Flag Register" (TIFR2) wird das "Timer Overflow Vector0" (TOV2) Bit bei einem Overflow von TCNT2 jetzt automatisch gesetzt.
    TOV2 wird automatisch gelöscht, wenn der Overflow Interrupt Vector ausgeführt wurde.
    siehe S.88

    kein allgemeiner Startwert für TCNT2, aber jedes mal, wenn logische 0, logische 1, oder Pause zwischen Bit58 und Bit0 entdeckt wurde sollte man zurücksetzen.

    alle hierfür verwendeten Bits sind in den Registern:
    TCCR2A
    TCCR0B
    TIMSK2
    TIFR2
  */
  //Diese Werte müssen die Register haben: (siehe Seite 127ff)
  TCCR2A = 0b00000000;
  TCCR2B = 0b00000111;
  TIMSK2 = 0b00000001;
  TCNT2 = 0;

}


volatile bool DCF77PinFlag = false;
//Interrupt Service Routine, der ausgeführt wird, wenn sich Pin 2 ändert.
ISR(INT0_vect) {
  DCF77PinFlag = true;
}

volatile bool ButtonFlag = false;
//Interrupt Service Routine, die jedes mal aktiviert wird, wenn einer der Knöpfe, mit der man die Uhr bedient wird gedrückt wird
ISR(INT1_vect) {
  ButtonFlag = true;
}

volatile bool Timer1Flag = false;
//Interrupt Service Routine, die ausgelöst wird, wenn TCNT1 einen Overflow-flag im TIFR1-Register hinterlässt
ISR(TIMER1_OVF_vect) {
  //TCNT1-counter muss in diesem Interrupt auf Startwert für 1000ms pro Durchlauf gesetzt werden!
  TCNT1 = 3036;
  Timer1Flag = true;
  //Serial.println("+");
}

volatile int Timer2OverflowCounter = 0;
//Interrupt Service Routine, der ausgelöst wird, wenn TCNT2 einen Overflow-flag im TIFR2-Register hinterlässt
ISR(TIMER2_OVF_vect) {
  Timer2OverflowCounter ++;
}




void loop() {


  //Wenn die Uhr im Funkuhr Modus ist zähle ich unabhängig von der Antenne die Sekunden mit
  //TCNT1 wird 1 mal mit dem Funksignal synchronisiert
  if (FunkuhrModus == true && Timer1Flag == true) {
    Timer1Flag = 0;
    SekundeVergangen ();
    UhrzeitAnLCDSenden (Stunde, Minute, Sekunde);
  }

  //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  //DCF77PinFlag wird im ISR auf 1 gesetzt (MC geht hier rein, wenn sich das Signal am Antennenpin ändert)
  if (DCF77PinFlag == true && FunkuhrModus == true) {
    DCF77PinFlag = 0;
    DCF77Signal = digitalRead(2);


    //vergangene Phase war ein Low
    //Beginn neuer Sekunde
    if (DCF77Signal == true) {
      DauerLow = Timer2OverflowCounter;
      //Serial.print("DauerLow:");
      //Serial.println(DauerLow);
      Timer2OverflowCounter = 0;
      TCNT2 = 0;

     
      //Beginn einer neuen Minute
      if (DauerLow > 100) {
        DCF77BitnummerZaehlen = true;
        DCF77ArrayInterpretieren();
        DCF77Bitnummer = 0;
        
        //Wenn die Uhr hier das erste mal ankommt wird angefangen die DCF77Bitnummer mit zu zählen
        //Wenn die Uhr hier zum zweiten mal ankommt muss die letzte Minute mit korekter Bitnummer mitgezählt geworden sein -> Uhrzeit fertig kalibiriert
        if (DCF77BitnummerZaehlen == true){
          DCF77FertigKalibriert = true;
        }
        DCF77BitnummerZaehlen = true;
      



        //Einmal in der Minute werden die Werte aus DCF77Interpretieren auf Minute, Stunde, Wochentag übertragen
        //So kann man verhindern, dass Wochentag, Stunde oder Minute Werte annimmt, die eigentlich gar nicht möglich sind. (z.b. 32:74)
        //Minute überprüfen
        if (FehlerAnAntenne == false) {
          Sekunde = 1;
        }
        if (MinuteDCF77 < 60 && FehlerAnAntenne == false) {
          Minute = MinuteDCF77;
        }
        //Stunde überprüfen
        if (StundeDCF77 < 24 && FehlerAnAntenne == false) {
          Stunde = StundeDCF77;
        }
        //Wochentag überprüfen
        if (WochentagDCF77 < 7 && FehlerAnAntenne == false) {
          Wochentag = WochentagDCF77;
        }

        Serial.println ("----- Beginn neuer Minute hier -----");
        Serial.print(Stunde);
        Serial.print(" : ");
        Serial.println(Minute);
        Serial.print("FehlerAnAntenne:");
        Serial.println(FehlerAnAntenne);

        //FehlerDauerDCF77Signal wird bei jeder Flanke von HIGH auf LOW an PIN2 überprüft. Kann da aber nur auf true gesetzt werden. Wenn FehlerDauerDCF77Signal == true würde es nirgendwo auf false gesetzt werden, falls dieser Fehler nicht existiert. Deswegen wird das einmal am Ende der Minute gemacht.
        FehlerDCF77HighDauer = false;
        FehlerDCF77LowDauer = false;
      }
    }



    //vergangene Phase war ein HIGH
    if (DCF77Signal == false) {
      DauerHigh = Timer2OverflowCounter;
      //Serial.print("DauerHigh:");
      //Serial.println(DauerHigh);
      Timer2OverflowCounter = 0;
      TCNT2 = 0;
      
      if (DauerHigh > 9) {
        DCF77Bitwert = 1;
      }
      else {
        DCF77Bitwert = 0;
      }
      DCF77ArrayMitschreiben ();
      if (DCF77BitnummerZaehlen == true) {
        DCF77Bitnummer++;
      }
            
      if (DauerHigh < 5) {
        FehlerDCF77HighDauer = true;
        Serial.println("jetzt Fehler High Dauer zu kurz!!");
        FehlerAnAntenne = FehlerBitnummer0 || FehlerBitnummer20 || FehlerParitaetStunde || FehlerParitaetStunde || FehlerParitaetDatum || FehlerDCF77HighDauer || FehlerDCF77LowDauer;
      }
      if (DauerHigh > 13) {
        FehlerDCF77HighDauer = true;
        Serial.println("jetzt Fehler High Dauer zu lang!!");
        FehlerAnAntenne = FehlerBitnummer0 || FehlerBitnummer20 || FehlerParitaetStunde || FehlerParitaetStunde || FehlerParitaetDatum || FehlerDCF77HighDauer || FehlerDCF77LowDauer;
      }
    }




  }


  //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  //Wenn die Uhr nicht im FunkuhrModus ist:
  if (FunkuhrModus == false && Timer1Flag == true) {
    Timer1Flag = 0;
    SekundeVergangen ();
    UhrzeitAnLCDSenden (Stunde, Minute, Sekunde);
  }


  //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  //zwischen Modi hin und her schalten
  if (ButtonFlag == true) {
    //if
  }


  //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  //Wenn man die Uhrzeit an der Uhr manuell einstellen möchte
  if (FunkuhrModus == false && ManuelEinstellen == true) {
    if (StundeEinstellen == 1 && KnopfFlag == 1) {
      UhrzeitAnLCDSenden (Stunde, Minute, Sekunde);
      lcd.setCursor(0, 1);
      lcd.print ("XX:--:--");
    }
    if (MinuteEinstellen == false && KnopfFlag == true) {
      UhrzeitAnLCDSenden (Stunde, Minute, Sekunde);
      lcd.setCursor(0, 1);
      lcd.print ("--:XX:--");
    }

    //sollte man auch noch Sekunde einstellen können?
    //Idee: wenn man in den Manuellen einstellen Modus geht: Sekunde = 0; ?

  }

}
