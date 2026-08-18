# G450_rotor

Modernisation d’un contrôleur de rotor **Yaesu G-450C** avec **ESP32-C3**.

<img src="projet.png" alt="Brochage ESP32-C3 SuperMini" width="600">
![Projet](images/projet.png)


Le projet permet de :
- conserver le pupitre/rotor d’origine,
- lire la position du rotor,
- commander la rotation **CW / CCW**,
- afficher l’azimut sur un **OLED SSD1306 128x64**,
- calibrer la recopie de position sur 5 points,
- sauvegarder la calibration en mémoire flash,
- piloter automatiquement le rotor vers une cible en azimut.

---

## Objectif du projet

Ce projet a pour but de réaliser un **contrôleur de rotor moderne** pour un **Yaesu G-450C**, avec une base simple, évolutive et facile à comprendre.

Le contrôleur est pensé pour :
- un usage radioamateur,
- une future liaison PC,
- une compatibilité à venir avec une logique de type **GS-232**,
- une évolution ultérieure possible vers un système **azimut + élévation**.

---

## État actuel du projet

Version actuelle : **V0.3.0**

Fonctions validées :
- lecture de la recopie de position,
- affichage de l’azimut,
- commande manuelle par boutons,
- calibration 5 points,
- sauvegarde de la calibration,
- asservissement automatique vers un angle cible,
- tolérance de position de **±4°**.

---

## Matériel utilisé

- **ESP32-C3 SuperMini**
- **OLED SSD1306 128x64** (I2C)
- **module 2 relais 5V** (commande active à l’état bas)
- **régulateur 7805**
- **2 boutons poussoirs manuels** : gauche / droite
- **1 bouton poussoir SETUP** : calibration
- alimentation externe
- câblage sur **plaque à trous** / veroboard

---

## Principe de fonctionnement

Le système n’attaque pas directement le moteur du rotor.

Il agit comme un **émulateur de boutons** :
- un relais simule la commande **CW**
- un relais simule la commande **CCW**

Le signal de **recopie de position** du G-450C est lu par l’ESP32-C3 via une entrée analogique, puis converti en angle.

---

## Fonctions disponibles

### Mode manuel
- bouton **Gauche** → rotation CCW
- bouton **Droite** → rotation CW

### Calibration
- appui long sur **SETUP**
- mémorisation des positions :
  - 0°
  - 90°
  - 180°
  - 270°
  - 360°

### Mode automatique

Commande série :
- `A0`
- `A90`
- `A180`
- `A270`
- `A360`

Exemple :

```text
A180
```

→ le rotor va automatiquement vers **180°**

Commande d’arrêt :

```text
S
```

→ arrêt de la commande automatique

---

## Tolérance

Le point cible est considéré comme atteint avec une tolérance de :

```text
±4°
```

Exemple :
- cible = `180°`
- position acceptée = `176° à 184°`

Une fois la cible atteinte, le rotor **s’arrête définitivement** et ne repart pas tout seul même si la mesure fluctue légèrement.

La tolérance est modifiable dans le code avec :

```cpp
const float TOLERANCE_ANGLE = 4.0;
```

---

## Calibration par défaut

Les valeurs actuellement utilisées comme calibration par défaut sont :

```cpp
uint16_t adcCalibration[NB_POINTS_CAL] =
{
  367,   //   0°
  774,   //  90°
  1145,  // 180°
  1462,  // 270°
  1786   // 360°
};
```

Ces valeurs peuvent ensuite être remplacées par une calibration réalisée directement sur le montage, puis sauvegardées en mémoire flash.

---

## Affectation des GPIO

<img src="images/esp32-c3-super-mini.png" alt="Brochage ESP32-C3 SuperMini" width="600">

### ESP32-C3 SuperMini

- `GPIO5` → relais CW
- `GPIO6` → relais CCW
- `GPIO2` → bouton gauche
- `GPIO1` → bouton droite
- `GPIO3` → bouton SETUP
- `GPIO0` → lecture recopie analogique
- `GPIO8` → SDA OLED
- `GPIO9` → SCL OLED

---

## Affichage OLED

L’écran affiche :
- l’azimut en grand,
- le sens de rotation (`<` ou `>`),
- l’état automatique (`AUTO xxx°`),
- l’indication `CIBLE OK` lorsque la cible est atteinte.

---

## Plateforme logicielle

Projet développé avec :
- **PlatformIO**
- framework **Arduino**
- langage **C++**

---

## Roadmap

### Fait

- [x] lecture analogique de la position
- [x] affichage OLED
- [x] commande manuelle
- [x] calibration 5 points
- [x] sauvegarde en mémoire flash
- [x] asservissement automatique azimut

### À faire

- [ ] liaison PC plus évoluée
- [ ] compatibilité **GS-232**
- [ ] amélioration de l’interface utilisateur
- [ ] boîtier final
- [ ] documentation câblage / photos
- [ ] évolution future vers axe élévation

---

## REMARQUES

Le projet est réalisé pour un usage personnel et expérimental, dans le but de moderniser un rotor Yaesu G-450C tout en gardant une architecture simple et évolutive.

---

## AUTEUR

Olivier / F4AFO

Projet développé avec l’aide de ChatGPT.
