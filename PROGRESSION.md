# Progression : SDL3 et jeux en C++

Dernière séance : 29/09/2026

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
| 4 | Textures (`05-textures`), texte SDL_ttf (`06-texte`), son (`07-son`) | ✅ Fait |
| 5 | États menu/partie/pause (`08-etats`), pas de temps fixe (`09-pas-fixe`) | ✅ Fait |
| 6 | Réseau avec SDL3_net : mini-chat TCP 🚧 (`10-chat`), puis protocole, UDP, jeu en réseau | ⏭️ **En cours** |

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

### Leçon 4 (suite) : texte et son (`06-texte/`, `07-son/`)
- Texte : `TTF_Init`, `TTF_OpenFont(chemin, taille)`, puis
  texte → `TTF_RenderText_Blended` → `SDL_Surface` (RAM) → `SDL_CreateTextureFromSurface` → texture (GPU).
  On détruit la surface tout de suite.
- **Cache avec invalidation** (`TexteCache { texture; valeur = -1; }`) : on ne régénère le texte que si le score change.
- Remplacement sûr : créer la nouvelle texture, **puis** détruire l'ancienne. Si la création échoue, on garde l'ancienne.
- Piège vu : le résultat de `SDL_CreateTextureFromSurface` non stocké = texture perdue **et** fuite GPU.
- Sorties anticipées (`if (...) return;`) plutôt que des `if` imbriqués.
- Son : `SDL_INIT_AUDIO`, `SDL_LoadWAV` (données libérées avec `SDL_free`), `SDL_OpenAudioDeviceStream`
  + `SDL_ResumeAudioStreamDevice`, et pour jouer : `SDL_ClearAudioStream` + `SDL_PutAudioStreamData`.
  Un flux est une **file** : sans mixage, les sons ne se superposent pas.
- Détecter un événement sans toucher à la logique : `const int scoreAvant = ...` **avant** `mettreAJour`,
  puis comparer après (piège vu : noter la valeur *après* = toujours égale).

### Leçon 5 : états (`08-etats/`)
- **Machine à états** : `enum class Ecran { Menu, Partie, Pause }` dans `AppState`.
  `enum class` : noms préfixés, pas de conversion implicite en `int`.
- Transitions dans `SDL_AppEvent` (actions ponctuelles), avec `switch` sur l'écran puis test de la touche.
  On ignore `event->key.repeat`. La même touche (Échap) fait des choses différentes selon l'écran.
- `||` seulement pour des conditions qui mènent à la **même** action (piège vu : Retour arrière rangé
  avec P/Échap envoyait vers la partie au lieu du menu).
- `mettreAJour` n'est appelée qu'en `Partie`, mais le `dt` est calculé à chaque frame (sinon énorme `dt` à la reprise).
- `SDL_RenderPresent` déplacé à la fin de `SDL_AppIterate`, pour dessiner les écrans par-dessus le jeu
  (voile semi-transparent avec `SDL_BLENDMODE_BLEND`, textes fixes générés une fois).

### Leçon 5 (suite) : pas de temps fixe (`09-pas-fixe/`)
- `PAS_FIXE = 1/60` dans `jeu.h`. Tirelire : `accumulateur += dt` (plafonné à 0.25 s contre la
  « spirale de la mort »), puis `while (accumulateur >= PAS_FIXE) { mettreAJour(..., PAS_FIXE); accumulateur -= PAS_FIXE; }`.
- Test sans écran : avec `dt` variable, 60 Hz et 144 Hz divergent. Avec le pas fixe, les positions
  sont **identiques au bit près pas par pas** (le 144 Hz peut juste avoir un pas de retard à un instant donné).
  → En réseau, on synchronise des **numéros de pas (ticks)**, pas des instants.
- Pas besoin de vider l'accumulateur à la reprise : il ne grossit que dans la branche `Partie`.

### Leçon 6 : réseau (`10-chat/`)
- **SDL3_net 3.2.0** (stable, mai 2026) n'est pas packagée sur Arch : elle est téléchargée et compilée par
  CMake (`FetchContent`, dans le `CMakeLists.txt` racine, en statique). Choisie plutôt qu'ENet pour la cohérence
  avec SDL3, TCP + UDP, et la simulation de perte de paquets intégrée.
- Notions : **IP** = l'immeuble (`127.0.0.1` = soi-même), **port** = l'appartement (on utilise 7777),
  **serveur** = attend les appels, **client** = appelle. **TCP** = appel téléphonique : connexion,
  fiable, ordonné, mais c'est un **flux d'octets** (pas de séparation entre les messages).
- Objets : `NET_Server` (le standard, ne sert qu'à décrocher), `NET_StreamSocket` (une ligne : envoyer et
  recevoir), `NET_Address`. Le client obtient son socket avec `NET_CreateClient`, l'hôte avec
  `NET_AcceptClient(serveur, &client)`.
- Tout est **non bloquant**. Pour attendre : `NET_WaitUntil...(..., -1)` (ok en terminal, interdit dans le jeu).
- Hôte : `NET_CreateServer(nullptr, PORT, 0)`, `NET_WaitUntilInputAvailable(tableau de void*, n, -1)`,
  `NET_AcceptClient` (gérer `false` **et** `client == nullptr`), `NET_GetStreamSocketAddress` (→ `NET_UnrefAddress` !).
- Lecture : `n = NET_ReadFromStreamSocket(sock, tampon, taille - 1)` ; `n > 0` → `tampon[n] = '\0'` ;
  `0` = rien pour l'instant ; `-1` = connexion cassée. Garder une case pour le `'\0'` (anti-débordement).
- Pièges vus : `%s` pour un entier (plantage), `%d` pour du texte ; noms d'API « traduits » en français ;
  détruire un objet qui vaut `nullptr` ; ne détruire que ce qui a été créé.
- Conseil : **compiler souvent**, même sans être sûr. Le compilateur est un assistant.

## Où on s'est arrêtés

Dans `10-chat/main.cpp`, section « Outils communs » : deux fonctions **à écrire** (le `TODO(human)`) :
- `envoyerTexte(socket, texte)` : `NET_WriteToStreamSocket(socket, texte, (int)SDL_strlen(texte))` et renvoyer son résultat ;
- `recevoirTexte(socket, tampon, tailleTampon)` : adapter l'ancien code (en commentaire dans la fonction) :
  `client` → `socket`, ne pas redéclarer `tampon`, `sizeof(tampon) - 1` → **`tailleTampon - 1`**
  (car `sizeof` d'un pointeur = 8 !), pas d'affichage du message, `return true` si `n > 0`, sinon `false`.
Les appels sont déjà en place : le client envoie, l'hôte répond « Bienvenue dans la salle ! », le client affiche la réponse.
Le programme compile avec des warnings tant que les deux fonctions sont vides (normal).
Test : terminal 1 `./build/10-chat/chat hote`, terminal 2 `./build/10-chat/chat client 127.0.0.1`.

Bonus facultatifs toujours en attente :
- marge autour du joueur et limite d'essais dans `genererPieces` ;
- flèches du clavier en plus de ZQSD dans `lireEntrees` (avec `||`).

## Prochaine étape

1. Écrire `envoyerTexte` / `recevoirTexte`, tester l'aller-retour.
2. Un vrai chat en boucle (taper des messages au clavier) → découvrir le problème du **flux TCP**
   (messages collés ou coupés) et inventer un **protocole** (préfixe de taille ou séparateur).
3. Envoyer des **structures** (les `Entrees` du jeu) au lieu de texte : sérialisation, ordre des octets.
4. UDP (`NET_DatagramSocket`), perte de paquets simulée, ticks.
5. Brancher dans le jeu : écrans Héberger / Rejoindre / Salle d'attente, version **non bloquante**.
