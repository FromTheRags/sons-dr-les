# Sons Drôles 🎵 (Branche SDL 3)

[![Build & Package](https://github.com/FromTheRags/sons-droles/actions/workflows/build.yml/badge.svg)](https://github.com/FromTheRags/sons-droles/actions/workflows/build.yml)
[![License: GPL-3.0-or-later](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Language: C++17](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://isocpp.org/)
[![Library: SDL 3](https://img.shields.io/badge/Library-SDL%203-green.svg)](https://www.libsdl.org/)
[![CMake](https://img.shields.io/badge/Build-CMake-064F8C.svg?logo=cmake&logoColor=white)](https://cmake.org/)

Portage natif et moderne de **Sons Drôles** vers **SDL 3** et **SDL3_ttf** :
- **Rendu GPU accéléré** avec présentation logique 840×680 et letterboxing automatique (`SDL_SetRenderLogicalPresentation`).
- **Moteur audio moderne SDL 3** basé sur `SDL_AudioStream` avec mixage multi-voix et streaming temps réel.
- **Conversion automatique des coordonnées curseur** (`SDL_ConvertEventToRenderCoordinates`).
- **Plein écran dynamique** sans étirement (`SDL_SetWindowFullscreen`).
- **CI/CD automatisée** avec compilation multiplateforme Ubuntu (Linux) et Windows (MinGW-w64 + Inno Setup).

---

## 📸 Captures d'écran

| Menu Principal (Pulsation & Niveaux) | Plateau de Jeu (Partie en cours) | Écran des Règles |
| :---: | :---: | :---: |
| <a href="Demo/menu.png"><img src="Demo/menu.png" alt="Menu Principal - Sélection de niveau" width="260" /></a> | <a href="Demo/ingame.png"><img src="Demo/ingame.png" alt="Partie en cours - Sons Drôles" width="260" /></a> | <a href="Demo/rules.png"><img src="Demo/rules.png" alt="Règles du jeu - Sons Drôles" width="260" /></a> |

---

## 🎮 Principe du jeu

Dans **Sons Drôles**, vous devez mémoriser et reconstituer des séquences de sons :
1. **Écoute** : Au début de chaque manche, la musique de fond se coupe et la séquence de 8 sons est jouée.
2. **Reconstitution** : Chaque case de la grille de 8 cases est associée à un son. Cliquez sur les cases dans le bon ordre pour reformer la séquence complète.
3. **Correction** : Vous pouvez annuler votre dernier choix en cliquant à nouveau sur la dernière case sélectionnée.
4. **Essais de combinaisons** : Vous disposez de **5 essais de combinaisons complètes** de 8 cases. Si votre suite de 8 cases est inexacte, la grille se réinitialise et vous perdez 1 essai plutôt que de subir une défaite immédiate.
5. **Réécoutes** : Vous disposez de **2 réécoutes** de la mélodie cible (bouton Replay en haut à droite avec badge).
6. **Chronomètre** : Vous avez 2 minutes pour réussir la grille.
7. **Progression** : Remportez les 4 premiers niveaux pour débloquer le **MODE EXTRÊME** (disposition et ordre entièrement aléatoires) !

### ⌨️ Raccourcis clavier
- **F11** ou **Alt + Entrée** : Activer / désactiver le plein écran (ratio d'aspect préservé)
- **Touche P** : Mettre en pause ou relancer la musique d'ambiance
- **Échap** : Revenir au menu principal ou quitter le jeu

---

## 🛠️ Compilation

### Prérequis Linux (Ubuntu / Debian)
```bash
sudo apt-get update
sudo apt-get install -y cmake ninja-build g++ pkg-config
# SDL 3 et SDL3_ttf peuvent être installés via paquets système ou téléchargés automatiquement via FetchContent dans CMake
```

### Compilation (Linux & macOS)
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/sons-droles
```

### Compilation sous Windows (CLion / MinGW / Visual Studio)
Le projet intègre automatiquement dans `cmake/sdl3/` les bibliothèques officielles de **SDL 3** et **SDL3_ttf** :
1. Ouvrez le dossier du projet dans **CLion** ou chargez le fichier `CMakeLists.txt`.
2. Configurez le profil de compilation (Release ou Debug).
3. Lancez la cible `sons-droles`. Les bibliothèques dynamiques (`SDL3.dll`, `SDL3_ttf.dll`) et les ressources (`assets/`) sont automatiquement copiées dans le répertoire de sortie de l'exécutable lors de la phase post-build.

---

## 📦 Structure du projet

```
.
├── .github/workflows/          # CI/CD GitHub Actions pour SDL 3 (Linux & Windows)
├── Demo/                       # Captures d'écran du jeu
├── assets/                     # Ressources du jeu libres de droits
│   ├── fonts/                  # Polices sous licence SIL Open Font License (OFL)
│   ├── images/                 # Arrière-plans et cases du jeu au format BMP
│   └── sounds/                 # Sons et musiques générés au format WAV (CC0/MIT)
├── cmake/
│   ├── sdl3/                   # Bibliothèques officielles SDL 3 et SDL3_ttf (Windows)
│   └── CopyRuntimeDependencies.cmake
├── innosetup/                  # Script d'installation Inno Setup pour Windows
├── source/                     # Code source C++17 moderne pour SDL 3
│   ├── assets.h / .cpp         # Gestionnaire de ressources et textures GPU
│   ├── audio.h / .cpp          # Moteur audio moderne SDL_AudioStream
│   ├── common.h                # Types globaux, constantes et en-têtes SDL 3
│   ├── game.h / .cpp           # Logique du jeu, règles, niveaux et timers
│   ├── renderer.h / .cpp       # Rendu accéléré GPU, formes géométriques et textes
│   └── main.cpp                # Boucle principale SDL 3 et boucle d'événements
├── tools/
│   └── generate_assets.py      # Script Python générateur d'assets (sons & images)
├── CMakeLists.txt              # Configuration CMake moderne multiplateforme SDL 3
├── LICENSE                     # Licence GNU GPLv3
└── README.md                   # Documentation du projet
```

---

## 📜 Licences
- **Code source** : Open-source sous licence [GNU General Public License v3.0 (GPLv3)](LICENSE), Copyright (C) 2013-2026 FromTheRags.
- **Polices** : Sous licence [SIL Open Font License 1.1](OFL.txt).
- **Graphismes et Sons** : Originaux et libres de droits.
