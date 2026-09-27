# Progression : SDL3 et jeux en C++

Dernière séance : 27/09/2026

Objectif final : des jeux PC en C++ avec SDL3 (API callbacks), jusqu'au multijoueur
(un joueur héberge une salle, les autres la rejoignent).

Compiler et lancer depuis `~/claudeCours/sdl3-jeux` :
```bash
cmake --build build && ./build/<dossier>/<exécutable>
```

## Plan du cours

| # | Leçon | État |
|---|-------|------|
| 1 | Fenêtre + boucle : callbacks (`01-fenetre`) et version classique (`01b-classique`) | ✅ Fait |
| 2 | Dessin, clavier, delta time, bords, diagonale (`02-mouvement`) | ✅ Fait |
| 3 | `std::vector`, collisions, pièces/score, séparation logique/SDL (`03-collisions`, `04-separation`) | ✅ Fait |
| 4 | Textures (SDL_image), texte (SDL_ttf), son | ⏭️ **Prochaine étape** |
| 5 | Architecture : états (menu/jeu/pause), pas de temps fixe | ⬜ À faire |
| 6 | Réseau : client/serveur, TCP vs UDP, héberger/rejoindre une salle, synchro | ⬜ À faire |

## Ce qu'on a vu

### Leçon 1 : fenêtre et boucle de jeu
- **Callbacks SDL3** : `SDL_AppInit` / `SDL_AppEvent` / `SDL_AppIterate` / `SDL_AppQuit`.
  `#define SDL_MAIN_USE_CALLBACKS` **avant** `#include <SDL3/SDL_main.h>`.
  Intérêt : SDL pilote la boucle, ce qui est indispensable pour le web (Emscripten) et iOS.
- État du programme dans une `struct AppState`, passée via `void** appstate` (pas de globales).
- `SDL_AppQuit` est appelé **même si Init échoue** : on attache l'état à SDL *avant* les étapes risquées.
- En SDL3, les fonctions renvoient un `bool` (`true` = succès). En cas d'échec, `SDL_GetError()` donne le message
  (à ne lire qu'après un échec).
- `SDL_AppInit` renvoie un `SDL_AppResult` (CONTINUE / SUCCESS / FAILURE), **pas** un `bool`.
- **Version classique** : `main()` + `while (enCours)` + `while (SDL_PollEvent(&event))` pour **vider** la file.
  On gère soi-même `SDL_Quit()` sur chaque chemin de sortie.
- Rendu : couleur (= état du renderer) → `Clear` → dessiner → `Present` (double buffering).

### Leçon 2 : mouvement
- Repère écran : origine en haut à gauche, **y vers le bas**.
- Événements (actions ponctuelles) contre `SDL_GetKeyboardState` (mouvement continu, sans délai de répétition).
- **Scancodes** = position physique : on écrit WASD en QWERTY, ce qui donne ZQSD sur AZERTY.
- **Delta time** : `Uint64 maintenant = SDL_GetTicksNS();` lu **une seule fois** ;
  `dt = (float)(maintenant - dernier) / SDL_NS_PER_SECOND`. On garde les timestamps en `Uint64`,
  car un `float` perd en précision sur les grandes valeurs.
- Bords : **bouger puis corriger** (sinon *tunneling* quand `dt` est grand).
- Diagonale : normaliser (dx, dy), en testant `norme > 0` (division par 0 → NaN contagieux).
  Piège vu : `dx + dy != 0` rate les diagonales (1, -1).
- Débogage : dérouler les valeurs à la main, `SDL_Log("dt = %f", dt)`.
- Optimisation : `if` / `std::clamp` / `SDL_clamp` donnent le même assembleur en `-O2`.
  Écrire lisible, mesurer avant d'optimiser.

### Leçon 3 : objets, collisions, architecture
- `std::vector` = tableau dynamique fait main + RAII : `push_back`, `size`, `[]`, `back`, `pop_back`, `empty`.
  Un `push_back` qui agrandit **invalide** les pointeurs et références vers les éléments.
- Foreach : `const T&` pour lire, `T&` pour modifier, `T` seulement si on veut une copie.
- **AABB** : collision = chevauchement en X **ET** en Y. Variante centres : `|cA - cB| <= wA/2 + wB/2`.
- **Supprimer pendant un parcours** : jamais `erase` dans un foreach (UB).
  Solution choisie : **swap-and-pop** en O(1), sans `++i` après une suppression, avec `i` en `size_t`.
- Référence vs objet : `p` (une `Piece`) sert à lire, `state->pieces` (le vector) à modifier.
- `do ... while` + **échantillonnage par rejet** pour ne pas faire apparaître de pièce sur le joueur.
- Initialisation avec accolades `Rect{...}` (C++11, anti-narrowing) plutôt que `Rect(...)` (agrégat, C++20 seulement).
- **Séparation** (`04-separation/`) : `jeu.h` / `jeu.cpp` sans SDL (`Jeu`, `Entrees` = 4 bool,
  `initialiser(jeu, graine)`, `mettreAJour(jeu, entrees, dt)`) ; `main.cpp` = `lireEntrees` → logique → `dessiner(const Jeu&)`.
  Hasard via `std::mt19937` avec graine : même graine = mêmes pièces, ce qui servira pour la synchro réseau.
  La logique tourne sans écran (base du futur serveur et des tests).

## Où on s'est arrêtés

Leçon 3 terminée et commitée (`04-separation/`). Le jeu : un carré rouge ramasse des pièces jaunes,
avec un score et des vagues infinies.

Bonus proposés, pas encore faits (facultatifs) :
- marge autour du joueur et limite d'essais dans `genererPieces` ;
- flèches du clavier en plus de ZQSD dans `lireEntrees` (avec `||`).

## Prochaine étape : leçon 4

Textures, texte et son, en partant de `04-separation/` (seul `main.cpp` et `dessiner` devraient changer) :
1. Charger une image avec **SDL_image** (`sdl3_image` est installé) : `IMG_LoadTexture`, `SDL_RenderTexture`.
   Détruire les textures dans `SDL_AppQuit`.
2. Texte avec **SDL_ttf** : **pas installé**, il faudra `sudo pacman -S sdl3_ttf` (vérifier le nom du paquet).
3. Son : l'API audio de SDL3 (`SDL_AudioStream`), sans bibliothèque en plus.
