/*
 * ============================================================
 *  YAESU G-450C - CONTROLEUR PC / GS-232
 *  ESP32-C3
 * ============================================================
 *
 *  Version : 0.2.3
 *  Date    : 17/08/2026
 *  Auteur  : F4AFO
 *
 *  Historique :
 *
 *  V0.1.0 - Validation materielle
 *           - Lecture position rotor
 *           - Commande CW / CCW
 *           - Boutons manuels
 *           - OLED SSD1306
 *           - Filtrage ADC
 *           - Interpolation 5 points provisoire
 *
 *  V0.2.0 - Calibration 5 points
 *           - Calibration 0 / 90 / 180 / 270 / 360 degres
 *           - Sauvegarde en memoire flash
 *           - Chargement automatique au demarrage
 *
 *  V0.2.1 - Bouton SETUP dedie
 *           - GPIO3
 *           - Entree calibration par appui long
 *           - Validation des points avec SETUP
 *
 *  V0.2.2 - Version diagnostic
 *           - Verification GPIO3 et bouton SETUP
 *           - Validation complete de la calibration
 *
 *  V0.2.3 - Version propre apres validation
 *           - Suppression du diagnostic bouton SETUP
 *           - Conservation calibration 5 points
 *           - Sauvegarde flash fonctionnelle
 *
 * ============================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>


// ============================================================
// ECRAN
// ============================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

Preferences preferences;


// ============================================================
// GPIO
// ============================================================

const int PIN_RELAIS_CW  = 5;
const int PIN_RELAIS_CCW = 6;

const int PIN_BOUTON_GAUCHE = 2;
const int PIN_BOUTON_DROITE = 1;
const int PIN_BOUTON_SETUP  = 3;

const int PIN_RECOPIE = 0;

const int PIN_SDA = 8;
const int PIN_SCL = 9;


// ============================================================
// CALIBRATION
// ============================================================

const int NB_POINTS_CAL = 5;

const int anglesCalibration[NB_POINTS_CAL] =
{
  0,
  90,
  180,
  270,
  360
};

uint16_t adcCalibration[NB_POINTS_CAL] =
{
  330,
  760,
  1138,
  1477,
  1775
};

bool calibrationValide = false;


// ============================================================
// VARIABLES
// ============================================================

float angleFiltre = 0;


// ============================================================
// COMMANDE ROTOR
// ============================================================

void stopRotor()
{
  digitalWrite(PIN_RELAIS_CW, HIGH);
  digitalWrite(PIN_RELAIS_CCW, HIGH);
}

void rotationCW()
{
  digitalWrite(PIN_RELAIS_CCW, HIGH);

  delay(20);

  digitalWrite(PIN_RELAIS_CW, LOW);
}

void rotationCCW()
{
  digitalWrite(PIN_RELAIS_CW, HIGH);

  delay(20);

  digitalWrite(PIN_RELAIS_CCW, LOW);
}


// ============================================================
// LECTURE ADC
// ============================================================

uint16_t lireADC()
{
  const int N = 64;

  uint32_t somme = 0;

  for (int i = 0; i < N; i++)
  {
    somme += analogRead(PIN_RECOPIE);

    delayMicroseconds(200);
  }

  return somme / N;
}


// ============================================================
// CONVERSION ADC -> ANGLE
// ============================================================

float adcVersAngle(uint16_t adc)
{
  if (adc <= adcCalibration[0])
  {
    return 0;
  }

  if (adc >= adcCalibration[NB_POINTS_CAL - 1])
  {
    return 360;
  }

  for (int i = 0;
       i < NB_POINTS_CAL - 1;
       i++)
  {
    if (
      adc >= adcCalibration[i] &&
      adc <= adcCalibration[i + 1]
    )
    {
      float proportion =
        (float)(adc - adcCalibration[i]) /
        (float)(
          adcCalibration[i + 1] -
          adcCalibration[i]
        );

      return
        anglesCalibration[i] +
        proportion *
        (
          anglesCalibration[i + 1] -
          anglesCalibration[i]
        );
    }
  }

  return 0;
}


// ============================================================
// FILTRAGE ANGLE
// ============================================================

float filtrerAngle(float nouvelAngle)
{
  angleFiltre =
    angleFiltre * 0.90 +
    nouvelAngle * 0.10;

  return angleFiltre;
}


// ============================================================
// AFFICHAGE NORMAL
// ============================================================

void afficher(float angle, const char *etat)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);

  display.setCursor(0, 0);

  display.print("AZIMUT");

  display.setTextSize(4);

  int angleAffiche = round(angle);

  if (strcmp(etat, "CCW") == 0)
  {
    display.setCursor(0, 18);

    display.print("<");

    display.print(angleAffiche);

    display.print((char)247);
  }

  else if (strcmp(etat, "CW") == 0)
  {
    display.setCursor(0, 18);

    display.print(angleAffiche);

    display.print((char)247);

    display.print(">");
  }

  else
  {
    display.setCursor(20, 18);

    display.print(angleAffiche);

    display.print((char)247);
  }

  display.setTextSize(1);

  display.setCursor(70, 52);

  display.print("F4AFO");

  display.display();
}


// ============================================================
// AFFICHAGE CALIBRATION
// ============================================================

void afficherCalibration(
  int point,
  uint16_t adc
)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);

  display.setCursor(0, 0);

  display.print("CALIBRATION");

  display.setCursor(0, 12);

  display.print("Position :");

  display.setTextSize(2);

  display.setCursor(0, 25);

  display.print(
    anglesCalibration[point]
  );

  display.print((char)247);

  display.setTextSize(1);

  display.setCursor(0, 50);

  display.print("ADC:");

  display.print(adc);

  display.setCursor(82, 50);

  display.print("SETUP");

  display.display();
}


// ============================================================
// SAUVEGARDE CALIBRATION
// ============================================================

void sauvegarderCalibration()
{
  preferences.begin(
    "rotor",
    false
  );

  for (int i = 0;
       i < NB_POINTS_CAL;
       i++)
  {
    char cle[8];

    sprintf(
      cle,
      "cal%d",
      i
    );

    preferences.putUInt(
      cle,
      adcCalibration[i]
    );
  }

  preferences.putBool(
    "valid",
    true
  );

  preferences.end();

  calibrationValide = true;
}


// ============================================================
// CHARGEMENT CALIBRATION
// ============================================================

void chargerCalibration()
{
  preferences.begin(
    "rotor",
    true
  );

  calibrationValide =
    preferences.getBool(
      "valid",
      false
    );

  if (calibrationValide)
  {
    for (int i = 0;
         i < NB_POINTS_CAL;
         i++)
    {
      char cle[8];

      sprintf(
        cle,
        "cal%d",
        i
      );

      adcCalibration[i] =
        preferences.getUInt(
          cle,
          adcCalibration[i]
        );
    }
  }

  preferences.end();
}


// ============================================================
// DETECTION ENTREE CALIBRATION
// ============================================================

bool demandeCalibration()
{
  static unsigned long debutAppui = 0;

  bool setupAppuye =
    digitalRead(
      PIN_BOUTON_SETUP
    ) == LOW;

  if (setupAppuye)
  {
    if (debutAppui == 0)
    {
      debutAppui = millis();
    }

    if (
      millis() - debutAppui
      >= 2000
    )
    {
      debutAppui = 0;

      return true;
    }
  }

  else
  {
    debutAppui = 0;
  }

  return false;
}


// ============================================================
// MODE CALIBRATION
// ============================================================

void modeCalibration()
{
  stopRotor();

  while (
    digitalRead(
      PIN_BOUTON_SETUP
    ) == LOW
  )
  {
    delay(10);
  }

  delay(300);

  for (int point = 0;
       point < NB_POINTS_CAL;
       point++)
  {
    bool pointValide = false;

    while (!pointValide)
    {
      bool gauche =
        digitalRead(
          PIN_BOUTON_GAUCHE
        ) == LOW;

      bool droite =
        digitalRead(
          PIN_BOUTON_DROITE
        ) == LOW;

      bool setup =
        digitalRead(
          PIN_BOUTON_SETUP
        ) == LOW;

      uint16_t adc =
        lireADC();

      afficherCalibration(
        point,
        adc
      );

      if (
        gauche &&
        !droite
      )
      {
        rotationCCW();
      }

      else if (
        droite &&
        !gauche
      )
      {
        rotationCW();
      }

      else
      {
        stopRotor();
      }

      if (setup)
      {
        stopRotor();

        delay(150);

        adcCalibration[point] =
          lireADC();

        Serial.print(
          "POINT "
        );

        Serial.print(
          anglesCalibration[point]
        );

        Serial.print(
          " deg memorise - ADC="
        );

        Serial.println(
          adcCalibration[point]
        );

        pointValide = true;

        while (
          digitalRead(
            PIN_BOUTON_SETUP
          ) == LOW
        )
        {
          delay(10);
        }

        delay(300);
      }

      delay(20);
    }

    display.clearDisplay();

    display.setTextColor(
      SSD1306_WHITE
    );

    display.setTextSize(1);

    display.setCursor(
      0,
      10
    );

    display.print(
      "POINT MEMORISE"
    );

    display.setTextSize(2);

    display.setCursor(
      20,
      28
    );

    display.print(
      anglesCalibration[point]
    );

    display.print(
      (char)247
    );

    display.display();

    delay(700);
  }


  // ==========================================================
  // VERIFICATION COHERENCE
  // ==========================================================

  bool valeursCorrectes = true;

  for (int i = 0;
       i < NB_POINTS_CAL - 1;
       i++)
  {
    if (
      adcCalibration[i + 1]
      <= adcCalibration[i]
    )
    {
      valeursCorrectes = false;
    }
  }


  // ==========================================================
  // SAUVEGARDE
  // ==========================================================

  if (valeursCorrectes)
  {
    sauvegarderCalibration();

    Serial.println(
      "CALIBRATION SAUVEGARDEE"
    );

    display.clearDisplay();

    display.setTextColor(
      SSD1306_WHITE
    );

    display.setTextSize(1);

    display.setCursor(
      15,
      18
    );

    display.print(
      "CALIBRATION"
    );

    display.setTextSize(2);

    display.setCursor(
      25,
      34
    );

    display.print("OK");

    display.display();
  }

  else
  {
    Serial.println(
      "ERREUR CALIBRATION : ADC NON CROISSANTS"
    );

    display.clearDisplay();

    display.setTextColor(
      SSD1306_WHITE
    );

    display.setTextSize(1);

    display.setCursor(
      10,
      18
    );

    display.print(
      "ERREUR CALIB."
    );

    display.setCursor(
      5,
      35
    );

    display.print(
      "Valeurs ADC"
    );

    display.setCursor(
      5,
      47
    );

    display.print(
      "non croissantes"
    );

    display.display();
  }

  stopRotor();

  delay(2000);

  angleFiltre =
    adcVersAngle(
      lireADC()
    );
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);


  // ----------------------------------------------------------
  // Relais
  // ----------------------------------------------------------

  pinMode(
    PIN_RELAIS_CW,
    OUTPUT
  );

  pinMode(
    PIN_RELAIS_CCW,
    OUTPUT
  );

  digitalWrite(
    PIN_RELAIS_CW,
    HIGH
  );

  digitalWrite(
    PIN_RELAIS_CCW,
    HIGH
  );


  // ----------------------------------------------------------
  // Boutons
  // ----------------------------------------------------------

  pinMode(
    PIN_BOUTON_GAUCHE,
    INPUT_PULLUP
  );

  pinMode(
    PIN_BOUTON_DROITE,
    INPUT_PULLUP
  );

  pinMode(
    PIN_BOUTON_SETUP,
    INPUT_PULLUP
  );


  // ----------------------------------------------------------
  // ADC
  // ----------------------------------------------------------

  analogReadResolution(12);


  // ----------------------------------------------------------
  // I2C
  // ----------------------------------------------------------

  Wire.begin(
    PIN_SDA,
    PIN_SCL
  );


  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  if (
    !display.begin(
      SSD1306_SWITCHCAPVCC,
      0x3C
    )
  )
  {
    Serial.println(
      "ERREUR OLED"
    );

    while (1)
    {
    }
  }


  // ----------------------------------------------------------
  // Calibration
  // ----------------------------------------------------------

  chargerCalibration();

  angleFiltre =
    adcVersAngle(
      lireADC()
    );

  display.clearDisplay();

  display.display();
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  // ----------------------------------------------------------
  // Entree calibration
  // ----------------------------------------------------------

  if (
    demandeCalibration()
  )
  {
    modeCalibration();

    return;
  }


  // ----------------------------------------------------------
  // Boutons manuels
  // ----------------------------------------------------------

  bool gauche =
    digitalRead(
      PIN_BOUTON_GAUCHE
    ) == LOW;

  bool droite =
    digitalRead(
      PIN_BOUTON_DROITE
    ) == LOW;

  const char *etat =
    "STOP";


  // ----------------------------------------------------------
  // Commande rotor
  // ----------------------------------------------------------

  if (
    gauche &&
    droite
  )
  {
    stopRotor();

    etat = "STOP";
  }

  else if (gauche)
  {
    rotationCCW();

    etat = "CCW";
  }

  else if (droite)
  {
    rotationCW();

    etat = "CW";
  }

  else
  {
    stopRotor();

    etat = "STOP";
  }


  // ----------------------------------------------------------
  // Position
  // ----------------------------------------------------------

  uint16_t adc =
    lireADC();

  float angleBrut =
    adcVersAngle(
      adc
    );

  float angle =
    filtrerAngle(
      angleBrut
    );


  // ----------------------------------------------------------
  // Affichage
  // ----------------------------------------------------------

  afficher(
    angle,
    etat
  );


  // ----------------------------------------------------------
  // Debug position
  // ----------------------------------------------------------

  Serial.print(
    "ADC="
  );

  Serial.print(
    adc
  );

  Serial.print(
    "  Angle="
  );

  Serial.println(
    angle,
    1
  );

  delay(50);
}