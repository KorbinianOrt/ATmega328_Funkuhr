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


//Mein Statusdiagramm hat 8 Status
bool Funkbetrieb = true;
bool Normalbetrieb = false;
bool WeckerAuswahl = false;
bool ManuellStundeEinstellen = false;
bool ManuellMinuteEinstellen = false;
bool WeckerTagEinstellen = false;
bool WeckerStundeEinstellen = false;
bool WeckerMinuteEinstellen = false;


//um mit den 6 Buttons zwischen 
enum class Betriebsart {
  Funkbetrieb,
  Normalbetrieb,
  Weckerauswahl,
  NeuerAlarm,
  WeckerTagEinstellen,
  WeckerStundeEinstellen,
  WeckerMinuteEinstellen,
  ManuellStundeEinstellen,
  ManuellMinuteEinstellen
};
Betriebsart aktuelleBetriebsart = Betriebsart::Normalbetrieb;


//eigentlich ist geplant, dass man beliebig viele Alarme einstellen kann
//wie ztum fick soll ich das denn machen????
//Bis ich das checke gibt's nur 3 Alarme
//Die Alarme brauchen ihre eigenen Attribute:
//Weckmethode?, An/aus?, AnWelchemTag aktiviert?, zu welcher Uhrzeit?
class KlasseAlarm{
  public:
  bool Aktiv;
  bool Licht;
  bool Ton;
  
  bool Montag;
  bool Dienstag;
  bool Mittwoch;
  bool Donnerstag;
  bool Freitag;
  bool Samstag;
  bool Sonntag;
  
  int Stunde;
  int Minute;
};

KlasseAlarm ObjektAlarm [50];
//beispiel:
//ObjektAlarm[nummer].Montag=true;



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
    if (Funkbetrieb == true) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("warte auf Signal");
    }

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
    lcd.print(" - ");

    switch (Wochentag) {
      case 1: lcd.print("Mo"); break;
      case 2: lcd.print("Di"); break;
      case 3: lcd.print("Mi"); break;
      case 4: lcd.print("Do"); break;
      case 5: lcd.print("Fr"); break;
      case 6: lcd.print("Sa"); break;
      case 7: lcd.print("So"); break;
    }
    //Was im FunkuhrModus angezeigt wird
    if (Funkbetrieb == true) {
      if (FehlerAnAntenne == true) {
        lcd.print(" -?");
      }
      else {
        lcd.print(" -#");
      }
    }
    else {
      lcd.print(" -0");
    }
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
      Minute = 0;
      Sekunde = 0;

      if (Stunde < 23) {
        Stunde++;
      }
      else {
        Stunde = 0;
        Minute = 0;
        Sekunde = 0;

        if (Wochentag < 7) {
          Wochentag++;
        }
        else {
          Wochentag = 0;
          Stunde = 0;
          Minute = 0;
          Sekunde = 0;
        }
      }
    }
  }
}



//A0 bis A5 sind die Buttons, mit denen der Wecker bedient wird
int const ButtonD = A0;
int const ButtonW = A1;
int const ButtonZ = A2;
int const ButtonPlus = A3;
int const ButtonMinus = A4;
int const ButtonLicht = A5;
void InputPinsLesen() {
  digitalRead(ButtonD);
  digitalRead(ButtonW);
  digitalRead(ButtonZ);
  digitalRead(ButtonPlus);
  digitalRead(ButtonMinus);
  digitalRead(ButtonLicht);
  if (digitalRead(ButtonD) == HIGH) {
    Serial.println("ButtonD gedrückt");
  }
  if (digitalRead(ButtonW) == HIGH) {
    Serial.println("ButtonW gedrückt");
  }
  if (digitalRead(ButtonZ) == HIGH) {
    Serial.println("ButtonZ gedrückt");
  }
  if (digitalRead(ButtonPlus) == HIGH) {
    Serial.println("ButtonPlus gedrückt");
  }
  if (digitalRead(ButtonMinus) == HIGH) {
    Serial.println("ButtonMinus gedrückt");
  }
  if (digitalRead(ButtonLicht) == HIGH) {
    Serial.println("ButtonLicht gedrückt");
  }
}



//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void setup() {

  Serial.begin(115200);
  Serial.println("hi");

  //Pin2 = Pin für die Antenne
  int const Antenne = 2;
  pinMode(Antenne, INPUT);

  //A0 bis A5 sind die Buttons, mit denen der Wecker bedient wird
  pinMode(ButtonD, INPUT);
  pinMode(ButtonW, INPUT);
  pinMode(ButtonZ, INPUT);
  pinMode(ButtonPlus, INPUT);
  pinMode(ButtonMinus, INPUT);
  pinMode(ButtonLicht, INPUT);




  //I-Bit wird gesetzt -> Interrupts sind global aktiviert (S. 15)
  sei();

  //Interrupt Pin = INT0 = Pin2 = PD2
  //Aktivierung von Pin2 als Interrup über die Bitmaske (s.55)
  EIMSK = 0b00000001;

  //Interrupt-Ereignis: Änderung von Pin2 (S.54)
  //ISC01=0 und ISC00=1
  EICRA = 0b00000001;


  lcd.begin(16, 2);
  lcd.print(" ^.^  - void #");
  lcd.setCursor(0, 1);
  lcd.print("o_0  - setup");
  delay (2000);
  lcd.clear();

  //16Bit Timer Register TCNT1: (für Sekunden)
  //Diese Werte müssen die Register haben: (siehe Seite 108 ff)
  TCCR1A = 0b00000000;
  TCCR1B = 0b00000100;
  TIMSK1 = 0b00000001;
  //TCNT1 Startwert:
  TCNT1 = 3036;

  //8Bit Timer Register TCNT2: (für Zeit zischen Änderungen am Antennenpin)
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

volatile bool ButtonFlag = true;
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



  //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  //zwischen Modi hin und her schalten und Uhrzeit und Wecker einstellen
  //Die 8 Status sind global definiert



  //nur gedacht, um zwischen den einzelnen Status hin und her zu wechseln
  if (ButtonFlag) {
    

    switch (aktuelleBetriebsart) {
      case Betriebsart::Funkbetrieb :
        if (digitalRead(ButtonD) == HIGH)
          aktuelleBetriebsart = Betriebsart::ManuellStundeEinstellen;
        else if (digitalRead(ButtonW) == HIGH)
          aktuelleBetriebsart = Betriebsart::Weckerauswahl;
        else if (digitalRead(ButtonPlus) == HIGH)
          aktuelleBetriebsart = Betriebsart::Normalbetrieb;
        break;
        
      case Betriebsart::Normalbetrieb :
        if (digitalRead(ButtonW) == HIGH)
          aktuelleBetriebsart = Betriebsart::Weckerauswahl;
        else if (digitalRead(ButtonD) == HIGH)
          aktuelleBetriebsart = Betriebsart::ManuellStundeEinstellen;
        else if (digitalRead(ButtonPlus) == HIGH)
          aktuelleBetriebsart = Betriebsart::Funkbetrieb;
        break;

      case Betriebsart::Weckerauswahl :
        if (digitalRead(ButtonZ) == HIGH)
          aktuelleBetriebsart = Betriebsart::Normalbetrieb;
        else if (digitalRead(ButtonMinus) == HIGH)
          aktuelleBetriebsart = Betriebsart::Weckerauswahl;
          //aktuell ausgewählten Wecker löschen/deaktivieren
        else if (digitalRead(ButtonW) == HIGH)
          aktuelleBetriebsart = Betriebsart::Weckerauswahl;
          //durchschalten zwischen WeckerAus, WeckerLicht, WeckerTon, WeckerTon & WeckerLicht
        else if (digitalRead(ButtonD) == HIGH)
          aktuelleBetriebsart = Betriebsart::Weckerauswahl;
          //durchschalten zwischen den einzelnen Alarmen (Alarm1 bis AlarmN, nach AlarmN: NeuerAlarm? Das dann mit + bestätigen
          //NeuerAlarm ist eigentlich hierfür gedacht
        else if (digitalRead(ButtonPlus) == HIGH)
          aktuelleBetriebsart = Betriebsart::WeckerTagEinstellen;
        break;

      case Betriebsart::WeckerTagEinstellen :

        break;

      case Betriebsart::WeckerStundeEinstellen :

        break;

      case Betriebsart::WeckerMinuteEinstellen :

        break;

      case Betriebsart::ManuellStundeEinstellen :

        break;

      case Betriebsart::ManuellMinuteEinstellen :

        break;

      default:
        break;
    }
  }



  //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  //Betrieb, wenn die Uhr im Funkuhr Modus ist zähle ich unabhängig von der Antenne die Sekunden mit
  //TCNT1 wird 1 mal mit dem Funksignal synchronisiert
  if (Funkbetrieb == true && Timer1Flag == true) {
    Timer1Flag = 0;
    SekundeVergangen ();
    UhrzeitAnLCDSenden (Stunde, Minute, Sekunde);
  }

  //DCF77PinFlag wird im ISR auf 1 gesetzt (MC geht hier rein, wenn sich das Signal am Antennenpin ändert)
  if (Funkbetrieb == true && DCF77PinFlag == true) {
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
        DCF77ArrayInterpretieren();
        DCF77Bitnummer = 0;

        //Wenn die Uhr hier das erste mal ankommt wird angefangen die DCF77Bitnummer mit zu zählen
        //Wenn die Uhr hier zum zweiten mal ankommt muss die letzte Minute mit korekter Bitnummer mitgezählt geworden sein -> Uhrzeit fertig kalibiriert
        if (DCF77BitnummerZaehlen == true && FehlerAnAntenne == false) {
          DCF77FertigKalibriert = true;
          TCNT0 = 0;
          //2.mal hier
        }
        DCF77BitnummerZaehlen = true;
        //1.mal hier



        //Einmal in der Minute werden die Werte aus DCF77Interpretieren auf Minute, Stunde, Wochentag übertragen
        //So kann man verhindern, dass Wochentag, Stunde oder Minute Werte annimmt, die eigentlich gar nicht möglich sind. (z.b. 32:74)
        //Minute überprüfen
        FehlerAnAntenne = FehlerBitnummer0 || FehlerBitnummer20 || FehlerParitaetStunde || FehlerParitaetStunde || FehlerParitaetDatum || FehlerDCF77HighDauer || FehlerDCF77LowDauer;
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
        //FehlerDCF77HighDauer bzw FehlerDCF77LowDauer wird bei jeder Flanke von HIGH auf LOW an PIN2 überprüft, kann da aber nur auf true gesetzt werden. Wenn FehlerDauerDCF77Signal == true würde es nirgendwo auf false gesetzt werden, falls dieser Fehler nicht existiert. Deswegen wird das einmal am Ende der Minute gemacht.
        FehlerDCF77HighDauer = false;
        FehlerDCF77LowDauer = false;


        Serial.println ("----- Beginn neuer Minute hier -----");
        Serial.print(Stunde);
        Serial.print(" : ");
        Serial.println(Minute);
        Serial.print("FehlerAnAntenne:");
        Serial.println(FehlerAnAntenne);

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
  //Betrieb, wenn die Uhr nicht im FunkuhrModus ist:
  if (Normalbetrieb == true && Timer1Flag == true) {
    Timer1Flag = 0;
    SekundeVergangen ();
    UhrzeitAnLCDSenden (Stunde, Minute, Sekunde);
  }



}
