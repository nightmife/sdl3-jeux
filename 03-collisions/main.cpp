// Leçon 3 : plusieurs objets (std::vector) et collisions.
// Le joueur doit ramasser des pièces réparties dans la fenêtre.

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <vector>

constexpr int LARGEUR_FENETRE = 1280;
constexpr int HAUTEUR_FENETRE = 720;

// En pixels PAR SECONDE : ne dépend plus du nombre de FPS
constexpr float VITESSE = 300.0f;

constexpr int NB_PIECES = 10;

struct Joueur {
    float x = 0.0f;  // position du coin haut-gauche
    float y = 0.0f;
    float taille = 50.0f;
};

struct Piece {
    float x = 0.0f;
    float y = 0.0f;
    float taille = 20.0f;
};

// Rectangle aligné sur les axes (AABB : Axis-Aligned Bounding Box)
struct Rect {
    float x, y;  // coin haut-gauche
    float w, h;  // largeur, hauteur
};

// Vrai si les deux rectangles se chevauchent (collision AABB).
// Deux rectangles se touchent s'ils se chevauchent sur l'axe X ET sur l'axe Y :
//   - sur X : le bord gauche de A est avant le bord droit de B,
//             ET le bord droit de A est après le bord gauche de B ;
//   - pareil sur Y.
// C'est un ET (&&) : se chevaucher sur un seul axe (même ligne ou même
// colonne) ne suffit pas. Avec <= / >=, se toucher pile par un bord compte.
static bool collision(const Rect& a, const Rect& b)
{
    return (a.x <= b.x + b.w && a.x + a.w >= b.x    // chevauchement horizontal
         && a.y <= b.y + b.h && a.y + a.h >= b.y);  // chevauchement vertical
}

// Ajoute NB_PIECES pièces à des positions aléatoires, jamais sur la zone interdite
// (le joueur). SDL_randf() renvoie un float dans [0, 1[, qu'on étire sur la zone
// où la pièce reste entièrement visible.
// Le vector est passé PAR RÉFÉRENCE (&) : sans ça, on remplirait une copie
// qui serait détruite à la fin de la fonction.
static void genererPieces(std::vector<Piece>& pieces, const Rect& zoneInterdite)
{
    for (int i = 0; i < NB_PIECES; ++i) {
        Piece p;
        // Échantillonnage par REJET : on tire une position, et si elle tombe sur
        // la zone interdite on recommence. do...while car il faut tirer AU MOINS
        // une fois avant de pouvoir tester.
        do {
            p.x = SDL_randf() * (LARGEUR_FENETRE - p.taille);
            p.y = SDL_randf() * (HAUTEUR_FENETRE - p.taille);
        } while (collision(zoneInterdite, Rect{p.x, p.y, p.taille, p.taille}));
        pieces.push_back(p);  // ajoute une copie de p à la fin du vector
    }
}

struct AppState {
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    Joueur        joueur;
    std::vector<Piece> pieces;
    int           score = 0;
    Uint64        dernierTemps = 0;  // instant de la frame précédente, en nanosecondes
};

SDL_AppResult SDL_AppInit(void** appstate, int /*argc*/, char* /*argv*/[])
{
    auto* state = new AppState{};
    *appstate = state;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init a échoué : %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Leçon 3 - Collisions", LARGEUR_FENETRE, HAUTEUR_FENETRE, 0,
                                     &state->window, &state->renderer)) {
        SDL_Log("Création de la fenêtre impossible : %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // VSync : on attend le rafraîchissement de l'écran avant chaque Present.
    // Le jeu tourne donc à la fréquence de ton écran (60 Hz, 144 Hz...).
    SDL_SetRenderVSync(state->renderer, 1);

    // Point de départ du chronomètre pour le calcul du delta time
    state->dernierTemps = SDL_GetTicksNS();

    // Joueur au centre de l'écran
    state->joueur.x = (LARGEUR_FENETRE - state->joueur.taille) / 2.0f;
    state->joueur.y = (HAUTEUR_FENETRE - state->joueur.taille) / 2.0f;

    Rect rJoueur{state->joueur.x, state->joueur.y, state->joueur.taille, state->joueur.taille};
    genererPieces(state->pieces, rJoueur);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* /*appstate*/, SDL_Event* event)
{
    if (event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;

    // Échap pour quitter, pratique pendant les tests
    if (event->type == SDL_EVENT_KEY_DOWN && event->key.scancode == SDL_SCANCODE_ESCAPE)
        return SDL_APP_SUCCESS;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    auto* state = static_cast<AppState*>(appstate);
    Joueur& joueur = state->joueur;

    // --- Mise à jour ---
    // Tableau de bool indexé par scancode : clavier[SDL_SCANCODE_X] vaut true
    // si la touche X est enfoncée EN CE MOMENT.
    const bool* clavier = SDL_GetKeyboardState(nullptr);
    
    // DELTA TIME : temps écoulé depuis la frame précédente, en secondes.
    // - Horloge lue UNE fois (sinon le temps entre deux lectures serait perdu).
    // - Uint64 et pas float : un float arrondirait ce grand nombre de ns.
    // - Soustraction en entier (exacte), PUIS conversion en float, PUIS division
    //   en float (une division entière donnerait 0).
    Uint64 maintenant = SDL_GetTicksNS();
    float dt = static_cast<float>(maintenant - state->dernierTemps) / SDL_NS_PER_SECOND;
    state->dernierTemps = maintenant;
    
    // Déplacement : direction (dx, dy) normalisée, puis vitesse * dt
    // 1. Direction voulue (if séparés : droite + gauche s'annulent).
    //    Scancodes = POSITION : WASD en QWERTY = ZQSD en AZERTY.
    float dx = 0.f, dy = 0.f;

    if (clavier[SDL_SCANCODE_D]) { dx += 1; } 
    if (clavier[SDL_SCANCODE_A]) { dx -= 1; }
    if (clavier[SDL_SCANCODE_W]) { dy -= 1; }
    if (clavier[SDL_SCANCODE_S]) { dy += 1; }

    // 2. Normalisation (sinon on va √2 fois plus vite en diagonale).
    //    On teste la norme : diviser par 0 donnerait NaN.
    float norme = SDL_sqrtf((dx * dx) + (dy * dy));
    if (norme > 0) {
        dx /= norme;
        dy /= norme;
    }

    // 3. Mouvement en pixels par SECONDE : indépendant des FPS
    joueur.x += VITESSE * dx * dt;
    joueur.y += VITESSE * dy * dt;

    // 4. Garder le joueur dans la fenêtre : bouger D'ABORD, corriger ENSUITE
    if (joueur.x > LARGEUR_FENETRE - joueur.taille) { joueur.x = LARGEUR_FENETRE - joueur.taille; }
    if (joueur.y > HAUTEUR_FENETRE - joueur.taille) { joueur.y = HAUTEUR_FENETRE - joueur.taille; }
    if (joueur.x < 0.f) { joueur.x = 0.f; }
    if (joueur.y < 0.f) { joueur.y = 0.f; }

    // Ramassage : retirer du vector les pièces touchées et augmenter le score
    // Swap-and-pop : on écrase la pièce i par la dernière puis on retire la
    // dernière (O(1), pas de décalage). Pas de i++ après une suppression : la
    // pièce arrivée en i doit encore être testée. Jamais d'erase dans un foreach.
    Rect rJoueur{joueur.x, joueur.y, joueur.taille, joueur.taille};  // hitbox du joueur
    size_t i = 0;  // size_t comme size() : pas de comparaison signé/non signé
    while (i < state->pieces.size()) {
        const Piece &p = state->pieces[i];  // const& : lire sans copier
        Rect rPiece{p.x, p.y, p.taille, p.taille};
        if (collision(rJoueur, rPiece)) {
            state->pieces[i] = state->pieces.back();
            state->pieces.pop_back();
            state->score += 1;
        }
        else { i++; }
    }

    // Nouvelle vague quand toutes les pièces sont ramassées
    if (state->pieces.empty()) { genererPieces(state->pieces, rJoueur); }

    // --- Dessin ---
    SDL_SetRenderDrawColor(state->renderer, 30, 30, 60, 255);
    SDL_RenderClear(state->renderer);

    // Pièces en jaune
    SDL_SetRenderDrawColor(state->renderer, 255, 200, 0, 255);
    // Couleur choisie UNE fois avant la boucle (c'est un état du renderer).
    // const Piece& : on lit chaque pièce sans la copier.
    for (const Piece& p : state->pieces) {
        SDL_FRect rect{p.x, p.y, p.taille, p.taille};
        SDL_RenderFillRect(state->renderer, &rect);
    }

    SDL_FRect rect{joueur.x, joueur.y, joueur.taille, joueur.taille};
    SDL_SetRenderDrawColor(state->renderer, 230, 80, 80, 255);
    SDL_RenderFillRect(state->renderer, &rect);

    // Score en haut à gauche (police 8x8 intégrée à SDL, pratique pour déboguer)
    SDL_SetRenderDrawColor(state->renderer, 255, 255, 255, 255);
    SDL_RenderDebugTextFormat(state->renderer, 10, 10, "Score : %d", state->score);

    SDL_RenderPresent(state->renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult /*result*/)
{
    auto* state = static_cast<AppState*>(appstate);
    if (!state)
        return;

    if (state->renderer) SDL_DestroyRenderer(state->renderer);
    if (state->window)   SDL_DestroyWindow(state->window);
    delete state;
}
