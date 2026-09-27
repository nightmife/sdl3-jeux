# Progression : SDL3 et jeux en C++

Dernière séance : 27/09/2026 (2e séance du jour)

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
| 4 | Textures (SDL_image) ✅ (`05-textures`), texte (SDL_ttf) 🚧 (`06-texte`), son ⬜ | ⏭️ **En cours** |
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

### Leçon 4 : textures (`05-textures/`)
- **Texture** = image dans la mémoire du GPU, liée au renderer. On la charge **une fois** (`IMG_LoadTexture`)
  dans Init, on la dessine à chaque frame, et on la détruit dans Quit **avant** le renderer.
- `SDL_RenderTexture(renderer, tex, srcrect, dstrect)` : `srcrect` = quelle partie de l'image
  (`nullptr` = tout ; servira pour les sprite sheets), `dstrect` = où et à quelle taille à l'écran.
- `SDL_FRect` (affichage, type SDL) ≠ `Rect` (logique / hitbox, notre type).
- Chemins : `SDL_GetBasePath()` (dossier de l'exe) + copie de `assets/` par CMake (`POST_BUILD`).
- Pixel art : `SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST)`, sinon l'image est floue.
- Plan B si l'image manque : rectangle de couleur. Piège vu : sans `SDL_SetRenderDrawColor`,
  le plan B dessinait avec la couleur du fond (invisible). Il faut **provoquer l'erreur exprès** pour tester.
- `if` invariant dans une boucle : pas de coût réel (prédiction de branchement, *loop unswitching*),
  donc on choisit selon la lisibilité.

## Où on s'est arrêtés

En plein milieu de l'étape **texte** (`06-texte/`, pas encore commitée au moment d'écrire ceci).
Tout est prêt (police `assets/police.ttf` = JetBrains Mono, `TTF_Init` / `TTF_OpenFont` / `TTF_CloseFont`,
`struct TexteCache { texture; valeur = -1; }`, affichage dans `dessiner`) **sauf** la fonction
`mettreAJourTexteScore`, que Dylan est en train d'écrire. Elle ne compile pas encore.

Principe : texte → `TTF_RenderText_Blended` → `SDL_Surface` (RAM) → `SDL_CreateTextureFromSurface`
→ `SDL_Texture` (GPU). C'est coûteux, donc on ne régénère que si le score change (cache + invalidation).

Plan de la fonction, et ce qu'il reste à corriger dans le brouillon actuel :
1. `if (police == nullptr) return;` puis `if (cache.valeur == score) return;`.
   ⚠️ Le brouillon **dessine** dans le cas sans police : à retirer, le plan B est déjà dans `dessiner`.
   ⚠️ Il utilise aussi `cache.score` au lieu de `cache.valeur`.
2. `std::string texte = "Score : " + std::to_string(score);` (déjà fait).
3. Surface via `TTF_RenderText_Blended(police, texte.c_str(), 0, SDL_Color{...})` (déjà fait),
   en vérifiant que la surface n'est pas `nullptr`.
4. ⚠️ Le résultat de `SDL_CreateTextureFromSurface` n'est **stocké nulle part** : il faut
   `SDL_Texture* nouvelle = ...`, puis `SDL_DestroySurface(surface)` dans tous les cas.
5. Détruire l'ancienne (`if (cache.texture) SDL_DestroyTexture(...)`), puis `cache.texture = nouvelle;`
   et `cache.valeur = score;` (la ligne `cache.valeur =` est incomplète).
   Question de conception en suspens : si la création échoue, garder l'ancienne texture ou non ?

Bonus facultatifs toujours en attente :
- marge autour du joueur et limite d'essais dans `genererPieces` ;
- flèches du clavier en plus de ZQSD dans `lireEntrees` (avec `||`).

## Prochaine étape

1. Terminer `mettreAJourTexteScore`, compiler, tester (le score s'affiche avec la vraie police).
   Tester aussi le plan B en renommant `build/06-texte/assets/police.ttf`.
2. Son : l'API audio de SDL3 (`SDL_AudioStream`, `SDL_LoadWAV`), sans bibliothèque en plus.
   Par exemple, un bruitage quand on ramasse une pièce (il faudra que la logique **signale** l'événement
   sans dépendre de SDL, par exemple avec un compteur de pièces ramassées pendant la frame).
3. Puis la leçon 5 (états menu/jeu/pause, pas de temps fixe).
