

//Library includes
#include <FastLED.h>
#include <TimeLib.h>
#include <DCF77.h>
#include <IRremote.h>
#include "RTClib.h"
#include "default_layout.h" //


// IR defines
#define ONOFF 0xFF02FD //set
#define AUTO 0xFF827D //set
#define RED 0xFF1AE5 //Set
#define GREEN 0xFF9A65 //Set
#define BLUE 0xFFA25D //Set
#define WHITE 0xFF22DD //Set
#define BRIGHTER 0xFF3AC5 //set
#define DIM 0xFFBA45//set

#define ORANGERED 0xFF2AD5
#define MEDIUMSPRINGGREEN 0xFFAA55
#define NAVY 0xFF926D
#define PINK 0xFF12ED
#define DARKORANGE 0xFF0AF5
#define SEAGREEN 0xFF8A75
#define GRAY 0xFFB24D
#define SALMON 0xFF32CD
#define CORAL 0xFF38C7
#define TEAL 0xFFB847
#define PURPLE 0xFF7887
#define LIGHTCYAN 0xFFF807
#define YELLOW 0xFF18E7
#define STEELBLUE 0xFF9867
#define MEDIUMVIOLETRED 0xFF58A7
#define LIGHTSKYBLUE 0xFFD827


#define QUICK 0xFFE817
#define SLOW 0xFFC837

#define DIY1 0xFF30CF
#define DIY2 0xFFB04F
#define DIY3 0xFF708F
#define FLASH 0xFFD02F
#define JUMP3 0xFF20DF
#define FADE3 0xFF609F
#define FADE7 0xFFE01F



//LED defines
#define NUM_LEDS 114

//PIN defines
#define STRIP_DATA_PIN 6
#define IR_RECV_PIN 11
#define ARDUINO_LED 13 //Default Arduino LED
#define DCF_PIN 2         // Connection pin to DCF 77 device
#define DCF_INTERRUPT 0		 // Interrupt number associated with pin
#define LDR_PIN 0

//dcf variables
DateTime time;
RTC_DS3231 rtc;
DCF77 DCF = DCF77(DCF_PIN, DCF_INTERRUPT);



uint8_t strip[NUM_LEDS];
uint8_t secondColorStrip[NUM_LEDS];
uint8_t stackptr = 0;

CRGB leds[NUM_LEDS];

IRrecv irrecv = IRrecv(IR_RECV_PIN);
decode_results irDecodeResults;


uint8_t selectedLanguageMode = 0;
const uint8_t RHEIN_RUHR_MODE = 0; //Define?
const uint8_t WESSI_MODE = 1;


boolean autoBrightnessEnabled = true;
boolean secondColor = false;
boolean testModus = false;
boolean randomColor = false;

uint8_t displayMode = 1;

CRGB defaultColor = CRGB::White;
CRGB secColor = CRGB::Red;
uint8_t colorIndex = 0;

int testHours = 0;
int testMinutes = 0;

//multitasking helper

const long oneSecondDelay = 1000;
const long halfSecondDelay = 500;

long waitUntilRtc = 0;
long waitUntilParty = 0;
long waitUntilOff = 0;
long waitUntilHeart = 0;
long waitUntilLDR = 0;

//forward declaration

void clockLogic();
void doIRLogic();
void doLDRLogic();
void makeParty();
void off();
void showHeart();
void pushToStrip(int ledId);
void resetAndBlack();
void resetStrip();
void displayStripRandomColor();
void displayStrip();
void displayStrip(CRGB colorCode);
void timeToStrip(uint8_t hours, uint8_t minutes);



//#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(str)  Serial.println (str)
#else
#define DEBUG_PRINT(str)
#endif

void setup() {

#ifdef DEBUG
  Serial.begin(9600);
#endif

  pinMode(ARDUINO_LED, OUTPUT);

  if (! rtc.begin()) {
    Serial.println("RTC is NOT running.");
  } else {
    Serial.println("RTC is running.");
  }

  //setup leds incl. fastled
  for (int i = 0; i < NUM_LEDS; i++) {
    strip[i] = 0;
  }
  FastLED.addLeds<WS2812B, STRIP_DATA_PIN, GRB>(leds, NUM_LEDS);
  resetAndBlack();
  displayStrip();

  //   setup dcf
  DCF.Start();
  setSyncInterval(60); //every hour
  setSyncProvider(getDCFTime);
  while (timeStatus() == timeNotSet) {
    // wait until the time is set by the sync provider
    delay(1000);
  }

  //setup ir
  irrecv.enableIRIn();
}

void loop() {
  doIRLogic();
  doLDRLogic();
  switch (displayMode) {
    case 0:
      off();
      break;
    case 1:
      clockLogic();
      break;
    case 2:
      makeParty();
      break;
    case 3:
      showHeart();
      break;
    default:
      clockLogic();
      break;
  }
}

unsigned long getDCFTime() {
  DEBUG_PRINT("Versucht zeit zu kriegen");
  time_t DCFtime = DCF.getTime();
  // Indicator that a time check is done
  if (DCFtime != 0) {
    rtc.adjust(DCFtime);
    setTime(DCFtime);
    if (testModus) {
      DEBUG_PRINT("Testmodus");
      leds[44] = CRGB::Green;
      FastLED.show();
    }
  }
  else {
    if (testModus) {
      leds[44] = CRGB::Red;
      FastLED.show();
    }
    time = rtc.now();
    setTime(time.unixtime());
  }
  return DCFtime;
}

void doLDRLogic() {

  if (millis() >= waitUntilLDR && autoBrightnessEnabled) {
    DEBUG_PRINT("LDR LOGIC");
    waitUntilLDR = millis();
    int ldrVal = map(analogRead(LDR_PIN), 0, 1023, 0, 150);
    FastLED.setBrightness(255 - ldrVal);
    FastLED.show();
    waitUntilLDR += oneSecondDelay;
  }
}

void doIRLogic() {

  uint8_t brightness = 0;
  if (irrecv.decode(&irDecodeResults)) {
    DEBUG_PRINT("IR LOGIC");
    switch (irDecodeResults.value) {
      case ONOFF:
        displayMode = 0;
        break;
      case AUTO:
        autoBrightnessEnabled = !autoBrightnessEnabled;
        break;
      case RED:
        defaultColor = CRGB::Red;
        displayStrip();
        break;
      case GREEN:
        defaultColor = CRGB::Green;
        displayStrip();
        break;
      case BLUE:
        defaultColor = CRGB::Blue;
        displayStrip();
        break;
      case WHITE:
        defaultColor = CRGB::White;
        displayStrip();
        break;
      case ORANGERED:
        defaultColor = CRGB::OrangeRed;
        displayStrip();
        break;
      case MEDIUMSPRINGGREEN:
        defaultColor = CRGB::MediumSpringGreen;
        displayStrip();
        break;
      case NAVY:
        defaultColor = CRGB::Navy;
        displayStrip();
        break;
      case PINK:
        defaultColor = CRGB::Pink;
        displayStrip();
        break;
      case DARKORANGE:
        defaultColor = CRGB::DarkOrange;
        displayStrip();
        break;
      case SEAGREEN:
        defaultColor = CRGB::SeaGreen;
        displayStrip();
        break;
      case GRAY:
        defaultColor = CRGB::Maroon;
        displayStrip();
        break;
      case SALMON:
        defaultColor = CRGB::Salmon;
        displayStrip();
        break;
      case CORAL:
        defaultColor = CRGB::Coral;
        displayStrip();
        break;
      case TEAL:
        defaultColor = CRGB::Teal;
        displayStrip();
        break;
      case PURPLE:
        defaultColor = CRGB::Purple;
        displayStrip();
        break;
      case LIGHTCYAN:
        defaultColor = CRGB::LightCyan;
        displayStrip();
        break;
      case YELLOW:
        defaultColor = CRGB::Yellow;
        displayStrip();
        break;
      case STEELBLUE:
        defaultColor = CRGB::SteelBlue;
        displayStrip();
        break;
      case MEDIUMVIOLETRED:
        defaultColor = CRGB::MediumVioletRed;
        displayStrip();
        break;
      case LIGHTSKYBLUE:
        defaultColor = CRGB::LightSkyBlue;
        displayStrip();
        break;
      case JUMP3:
        randomColor = !randomColor;
        testMinutes = -1;
        testHours = -1;
        break;
      case BRIGHTER:
        autoBrightnessEnabled = false;
        brightness = FastLED.getBrightness();
        if (brightness <= 255 - 50) {
          FastLED.setBrightness(brightness + 50);
        } else {
          FastLED.setBrightness(255);
        }
        FastLED.show();
        break;
      case DIM:
        autoBrightnessEnabled = false;
        brightness = FastLED.getBrightness();
        if (brightness >= 50) {
          FastLED.setBrightness(brightness - 50);
        } else {
          FastLED.setBrightness(0);
        }
        FastLED.show();
        break;
      case DIY1:
        displayMode = 1;
        autoBrightnessEnabled = true;
        //to force display update
        testMinutes = -1;
        testHours = -1;
        break;
      case DIY2:
        displayMode = 2;
        break;
      case DIY3:
        displayMode = 3;
        break;
      case FLASH:
        testMinutes = -1;
        testHours = -1;
        DEBUG_PRINT("SecondColor");
        secondColor = !secondColor;
        clockLogic();
        break;
      case QUICK:
        testMinutes = -1;
        testHours = -1;
        secColor = nextColor();
        clockLogic();
        break;
      case SLOW:
        testMinutes = -1;
        testHours = -1;
        secColor = prevColor();
        clockLogic();
        break;
      case FADE7:
        leds[44] = CRGB::Yellow;
        FastLED.show();
        testModus = !testModus;
        break;
      case FADE3:
        defaultColor = CRGB::Black;
        displayStrip();
        break;
      default:
        DEBUG_PRINT("IR DEFAULT");
        break;
    }
    irrecv.resume();
  }
}


///////////////////////
//DISPLAY MODES
///////////////////////
void clockLogic() {

  if (millis() >= waitUntilRtc) {
    DEBUG_PRINT("CLOCK LOGIC");
    //  DEBUG_PRINT("doing clock logic");
    waitUntilRtc = millis();
    if (testMinutes != minute() || testHours != hour()) {
      testMinutes = minute();
      testHours = hour();
      resetAndBlack();
      timeToStrip(testHours, testMinutes);
      if (secondColor) {
        DEBUG_PRINT("SECONDCOLOR");
        for (int i = 0; i < stackptr; i++) {
          secondColorStrip[strip[i]] = 1;
        }
        for (int i = 0; i < NUM_LEDS; i++) {
          if (secondColorStrip[i] == 0) {
            leds[i] = secColor;
          }
        }
      }
      if (randomColor) {
        DEBUG_PRINT("RANDOM COLOR");
        displayStripRandomColor();
      }
      else {
        displayStrip(defaultColor);
      }

    }

    waitUntilRtc += oneSecondDelay;
  }
}

void off() {


  if (millis() >= waitUntilOff) {
    secondColor = false;
    testModus = false;
    DEBUG_PRINT("OFF");
    //  DEBUG_PRINT("switching off");
    waitUntilOff = millis();
    resetAndBlack();
    displayStrip(CRGB::Black);
    waitUntilOff += halfSecondDelay;
  }
}


void makeParty() {
  
  if (millis() >= waitUntilParty) {
    DEBUG_PRINT("PARTY");
    autoBrightnessEnabled = false;
    //  DEBUG_PRINT("YEAH party party");
    waitUntilParty = millis();
    resetAndBlack();
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(random(0, 255), 255, 255);
    }
    FastLED.show();
    waitUntilParty += halfSecondDelay;
  }
}



void showHeart() {
  
  if (millis() >= waitUntilHeart) {
    DEBUG_PRINT("HEART");
    autoBrightnessEnabled = false;
    //   DEBUG_PRINT("showing heart");
    waitUntilHeart = millis();
    resetAndBlack();
    pushToStrip(L29); pushToStrip(L30); pushToStrip(L70); pushToStrip(L89);
    pushToStrip(L11); pushToStrip(L48); pushToStrip(L68); pushToStrip(L91);
    pushToStrip(L7); pushToStrip(L52); pushToStrip(L107);
    pushToStrip(L6); pushToStrip(L106);
    pushToStrip(L5); pushToStrip(L105);
    pushToStrip(L15); pushToStrip(L95);
    pushToStrip(L23); pushToStrip(L83);
    pushToStrip(L37); pushToStrip(L77);
    pushToStrip(L41); pushToStrip(L61);
    pushToStrip(L59);
    displayStrip(CRGB::Red);
    waitUntilHeart += oneSecondDelay;
  }
}

///////////////////////

CRGB prevColor() {
  if (colorIndex > 0) {
    colorIndex--;
  }
  return getColorForIndex();
}
CRGB nextColor() {
  if (colorIndex < 9) {
    colorIndex++;
  }
  return getColorForIndex();
}

CRGB getColorForIndex() {
  switch (colorIndex) {
    case 0:
      return CRGB::White;
    case 1:
      return CRGB::Blue;
    case 2:
      return CRGB::Aqua;
    case 3:
      return CRGB::Green;
    case 4:
      return CRGB::Lime;
    case 5:
      return CRGB::Red;
    case 6:
      return CRGB::Magenta;
    case 7:
      return CRGB::Olive;
    case 8:
      return CRGB::Yellow;
    case 9:
      return CRGB::Silver;
    default:
      colorIndex = 0;
      return CRGB::White;
  }
}

void pushToStrip(int ledId) {
  strip[stackptr] = ledId;
  stackptr++;
}

void resetAndBlack() {
  resetStrip();
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
}

void resetStrip() {
  stackptr = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    strip[i] = 0;
    secondColorStrip[i] = 0;
  }
}

void displayStripRandomColor() {
  for (int i = 0; i < stackptr; i++) {
    leds[strip[i]] = CHSV(random(0, 255), 255, 255);
  }
  FastLED.show();
}

void displayStrip() {
  displayStrip(defaultColor);
}

void displayStrip(CRGB colorCode) {
  for (int i = 0; i < stackptr; i++) {
    leds[strip[i]] = colorCode;
  }
  FastLED.show();
}

void timeToStrip(uint8_t hours, uint8_t minutes)
{
  pushES_IST();

  //show minutes
  if (minutes >= 5 && minutes < 10) {
    pushFUENF1();
    pushNACH();
  } else if (minutes >= 10 && minutes < 15) {
    pushZEHN1();
    pushNACH();
  } else if (minutes >= 15 && minutes < 20) {
    pushVIERTEL();
    pushNACH();
  } else if (minutes >= 20 && minutes < 25) {
    if (selectedLanguageMode == RHEIN_RUHR_MODE) {
      pushZWANZIG();
      pushNACH();
    } else if (selectedLanguageMode == WESSI_MODE) {
      pushZEHN1();
      pushVOR();
      pushHALB();
    }
  } else if (minutes >= 25 && minutes < 30) {
    pushFUENF1();
    pushVOR();
    pushHALB();
  } else if (minutes >= 30 && minutes < 35) {
    pushHALB();
  } else if (minutes >= 35 && minutes < 40) {
    pushFUENF1();
    pushNACH();
    pushHALB();
  } else if (minutes >= 40 && minutes < 45) {
    if (selectedLanguageMode == RHEIN_RUHR_MODE) {
      pushZWANZIG();
      pushVOR();
    } else if (selectedLanguageMode == WESSI_MODE) {
      pushZEHN1();
      pushNACH();
      pushHALB();
    }
  } else if (minutes >= 45 && minutes < 50) {
    pushVIERTEL();
    pushVOR();
  } else if (minutes >= 50 && minutes < 55) {
    pushZEHN1();
    pushVOR();
  } else if (minutes >= 55 && minutes < 60) {
    pushFUENF1();
    pushVOR();
  }

  int singleMinutes = minutes % 5;
  switch (singleMinutes) {
    case 1:
      pushONE();
      break;
    case 2:
      pushONE();
      pushTWO();
      break;
    case 3:
      pushONE();
      pushTWO();
      pushTHREE();
      break;
    case 4:
      pushONE();
      pushTWO();
      pushTHREE();
      pushFOUR();
      break;
  }

  if (hours >= 12) {
    hours -= 12;
  }

  if (selectedLanguageMode == RHEIN_RUHR_MODE) {
    if (minutes >= 25) {
      hours++;
    }
  } else if (selectedLanguageMode == WESSI_MODE) {
    if (minutes >= 20) {
      hours++;
    }
  }

  if (hours == 12) {
    hours = 0;
  }

  //show hours
  switch (hours) {
    case 0:
      pushZWOELF();
      break;
    case 1:
      if (minutes > 4) {
        pushEINS(true);
      } else {
        pushEINS(false);
      }
      break;
    case 2:
      pushZWEI();
      break;
    case 3:
      pushDREI();
      break;
    case 4:
      pushVIER();
      break;
    case 5:
      pushFUENF2();
      break;
    case 6:
      pushSECHS();
      break;
    case 7:
      pushSIEBEN();
      break;
    case 8:
      pushACHT();
      break;
    case 9:
      pushNEUN();
      break;
    case 10:
      pushZEHN();
      break;
    case 11:
      pushELF();
      break;
  }

  //show uhr
  if (minutes < 5) {
    pushUHR();
  }

}

///////////////////////
//PUSH WORD HELPER
///////////////////////
void pushES_IST()  {
  pushToStrip(L9);
  pushToStrip(L10);
  pushToStrip(L30);
  pushToStrip(L49);
  pushToStrip(L50);
}

void pushFUENF1() {
  pushToStrip(L70);
  pushToStrip(L89);
  pushToStrip(L90);
  pushToStrip(L109);
}

void pushFUENF2() {
  pushToStrip(L74);
  pushToStrip(L85);
  pushToStrip(L94);
  pushToStrip(L105);
}

void pushNACH() {
  pushToStrip(L73);
  pushToStrip(L86);
  pushToStrip(L93);
  pushToStrip(L106);
}

void pushZEHN1() {
  pushToStrip(L8);
  pushToStrip(L11);
  pushToStrip(L28);
  pushToStrip(L31);
}

void pushVIERTEL() {
  pushToStrip(L47);
  pushToStrip(L52);
  pushToStrip(L67);
  pushToStrip(L72);
  pushToStrip(L87);
  pushToStrip(L92);
  pushToStrip(L107);
}

void pushVOR() {
  pushToStrip(L6);
  pushToStrip(L13);
  pushToStrip(L26);
}

void pushHALB() {
  pushToStrip(L5);
  pushToStrip(L14);
  pushToStrip(L25);
  pushToStrip(L34);
}

void pushONE() {
  pushToStrip(L113);
}

void pushTWO() {
  pushToStrip(L110);
}

void pushTHREE() {
  pushToStrip(L111);
}

void pushFOUR() {
  pushToStrip(L112);
}

void pushZWANZIG() {
  pushToStrip(L48);
  pushToStrip(L51);
  pushToStrip(L68);
  pushToStrip(L71);
  pushToStrip(L88);
  pushToStrip(L91);
  pushToStrip(L108);
}

void pushZWOELF() {
  pushToStrip(L61);
  pushToStrip(L78);
  pushToStrip(L81);
  pushToStrip(L98);
  pushToStrip(L101);
}

void pushEINS(bool s) {
  pushToStrip(L4);
  pushToStrip(L15);
  pushToStrip(L24);
  if (s) {
    pushToStrip(L35);
  }
}

void pushZWEI() {
  pushToStrip(L75);
  pushToStrip(L84);
  pushToStrip(L95);
  pushToStrip(L104);
}

void pushDREI() {
  pushToStrip(L3);
  pushToStrip(L16);
  pushToStrip(L23);
  pushToStrip(L36);
}

void pushVIER() {
  pushToStrip(L76);
  pushToStrip(L83);
  pushToStrip(L96);
  pushToStrip(L103);
}

void pushSECHS() {
  pushToStrip(L2);
  pushToStrip(L17);
  pushToStrip(L22);
  pushToStrip(L37);
  pushToStrip(L42);
}

void pushSIEBEN() {
  pushToStrip(L1);
  pushToStrip(L18);
  pushToStrip(L21);
  pushToStrip(L38);
  pushToStrip(L41);
  pushToStrip(L58);
}

void pushACHT() {
  pushToStrip(L77);
  pushToStrip(L82);
  pushToStrip(L97);
  pushToStrip(L102);
}

void pushNEUN() {
  pushToStrip(L39);
  pushToStrip(L40);
  pushToStrip(L59);
  pushToStrip(L60);
}

void pushZEHN() {
  pushToStrip(L0);
  pushToStrip(L19);
  pushToStrip(L20);
  pushToStrip(L39);
}

void pushELF() {
  pushToStrip(L54);
  pushToStrip(L65);
  pushToStrip(L74);
}

void pushUHR() {
  pushToStrip(L80);
  pushToStrip(L99);
  pushToStrip(L100);
}
///////////////////////
