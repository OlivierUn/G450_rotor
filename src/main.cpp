#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// GPIO
const int PIN_RELAIS_CW  = 5;
const int PIN_RELAIS_CCW = 6;

const int PIN_BOUTON_GAUCHE = 2;
const int PIN_BOUTON_DROITE = 1;

const int PIN_RECOPIE = 0;

// I2C
const int PIN_SDA = 8;
const int PIN_SCL = 9;

// Calibration provisoire basée sur tes mesures
const float VOLT_0   = 0.276;
const float VOLT_360 = 1.430;

void stopRotor()
{
  digitalWrite(PIN_RELAIS_CW, HIGH);
  digitalWrite(PIN_RELAIS_CCW, HIGH);
}

void rotationCW()
{
  // sécurité : couper l'autre sens avant
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

float lireTensionRecopie()
{
  const int N = 64;
  uint32_t somme = 0;

  for (int i = 0; i < N; i++)
  {
    somme += analogRead(PIN_RECOPIE);
    delayMicroseconds(200);
  }

  float valeurADC = somme / (float)N;

  return valeurADC * 3.3 / 4095.0;
}

float angleFiltre = 0;

float filtrerAngle(float nouvelAngle)
{
  angleFiltre = angleFiltre * 0.90 + nouvelAngle * 0.10;
  return angleFiltre;
}

float tensionVersAngle(float v)
{
  const float volts[]  = {0.268, 0.613, 0.917, 1.190, 1.430};
  const float angles[] = {0, 90, 180, 270, 360};

  if (v <= volts[0])
    return 0;

  if (v >= volts[4])
    return 360;

  for (int i = 0; i < 4; i++)
  {
    if (v >= volts[i] && v <= volts[i + 1])
    {
      float proportion =
        (v - volts[i]) /
        (volts[i + 1] - volts[i]);

      return angles[i] +
             proportion *
             (angles[i + 1] - angles[i]);
    }
  }

  return 0;
}

void afficher(float angle, const char *etat)
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Petite ligne en haut
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("AZIMUT");

  // Grande valeur
  display.setTextSize(4);

  int angleAffiche = round(angle);

  if (strcmp(etat, "CCW") == 0)
  {
    display.setCursor(0, 18);
    display.print("< ");
    display.print(angleAffiche);
    display.print((char)247);
  }
  else if (strcmp(etat, "CW") == 0)
  {
    display.setCursor(0, 18);
    display.print(angleAffiche);
    display.print((char)247);
    display.print(" >");
  }
  else
  {
    display.setCursor(20, 18);
    display.print(angleAffiche);
    display.print((char)247);
  }

    // Indicatif en bas
  display.setTextSize(1);
  display.setCursor(70, 52);
  display.print("F4AFO");

  display.display();
}

void setup()
{
  Serial.begin(115200);

  // Très important :
  // relais OFF immédiatement au démarrage
  pinMode(PIN_RELAIS_CW, OUTPUT);
  pinMode(PIN_RELAIS_CCW, OUTPUT);

  digitalWrite(PIN_RELAIS_CW, HIGH);
  digitalWrite(PIN_RELAIS_CCW, HIGH);

  pinMode(PIN_BOUTON_GAUCHE, INPUT_PULLUP);
  pinMode(PIN_BOUTON_DROITE, INPUT_PULLUP);

  analogReadResolution(12);

  Wire.begin(PIN_SDA, PIN_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    while (1);
  }

  display.clearDisplay();
  display.display();
}

void loop()
{
  bool gauche =
      digitalRead(PIN_BOUTON_GAUCHE) == LOW;

  bool droite =
      digitalRead(PIN_BOUTON_DROITE) == LOW;

const char *etat = "STOP";

if (gauche && droite)
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
}

float tension = lireTensionRecopie();
float angleBrut = tensionVersAngle(tension);
float angle = filtrerAngle(angleBrut);


  afficher(angle, etat);

  Serial.print("ADC=");
  Serial.print(analogRead(PIN_RECOPIE));

  Serial.print("  V=");
  Serial.print(tension, 3);

  Serial.print("  Angle=");
  Serial.println(angle, 1);

  delay(100);
}