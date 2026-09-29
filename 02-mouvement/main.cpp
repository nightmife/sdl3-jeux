// Leçon 2 : dessiner un joueur et le déplacer au clavier.
// Étape 1 : déplacement "naïf", d'un nombre fixe de pixels par frame.
// Étape 2 : déplacement en pixels par SECONDE grâce au delta time.

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

constexpr int LARGEUR_FENETRE = 1280;
constexpr int HAUTEUR_FENETRE = 720;

// En pixels PAR SECONDE : ne dépend plus du nombre de FPS
constexpr float VITESSE = 300.0f;

struct Joueur {
    float x = 0.0f;  // position du coin haut-gauche
    float y = 0.0f;
    float taille = 50.0f;
};

struct AppState {
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    Joueur        joueur;
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

    if (!SDL_CreateWindowAndRenderer("Leçon 2 - Mouvement", LARGEUR_FENETRE, HAUTEUR_FENETRE, 0,
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
    // - On lit l'horloge UNE seule fois, sinon le temps entre deux lectures serait perdu.
    // - "maintenant" reste un Uint64 : un float n'a que ~7 chiffres significatifs et
    //   arrondirait de plus en plus grossièrement ce grand nombre de nanosecondes.
    // - On soustrait d'abord (en entier, exact), on convertit en float ensuite
    //   (la différence est petite, donc sans perte), puis on divise en float
    //   (une division entière donnerait 0).
    Uint64 maintenant = SDL_GetTicksNS();
    float dt = static_cast<float>(maintenant - state->dernierTemps) / SDL_NS_PER_SECOND;
    state->dernierTemps = maintenant;  // APRÈS le calcul de dt, pour la frame suivante

    // Déplacement : direction (dx, dy) normalisée, puis vitesse * dt
    // 1. Direction voulue. Des if SÉPARÉS (pas de else) : droite + gauche
    //    s'annulent, haut + droite donnent une diagonale.
    //    Scancodes = POSITION de la touche (WASD en QWERTY = ZQSD en AZERTY).
    float dx = 0.f, dy = 0.f;
    if (clavier[SDL_SCANCODE_D]) { dx += 1; }
    if (clavier[SDL_SCANCODE_A]) { dx -= 1; }
    if (clavier[SDL_SCANCODE_W]) { dy -= 1; }  // y va vers le BAS à l'écran : monter = y diminue
    if (clavier[SDL_SCANCODE_S]) { dy += 1; }

    // 2. Normalisation : en diagonale (1, 1) a une longueur de √2 ≈ 1,41, on irait
    //    41 % plus vite. On divise par la longueur pour la ramener à 1.
    //    On teste la NORME (et pas dx + dy, qui vaut 0 pour la diagonale (1, -1)) :
    //    diviser par 0 donnerait NaN, qui "contamine" tous les calculs suivants.
    float norme = SDL_sqrtf((dx * dx) + (dy * dy));
    if (norme > 0) {
        dx /= norme;
        dy /= norme;
    }

    // 3. Mouvement : VITESSE est en pixels PAR SECONDE, donc × dt (en secondes)
    //    donne les pixels à parcourir cette frame, quel que soit le nombre de FPS.
    joueur.x += VITESSE * dx * dt;
    joueur.y += VITESSE * dy * dt;

    // 4. Garder le joueur dans la fenêtre : on BOUGE D'ABORD, puis on CORRIGE.
    //    (Tester avant de bouger laisserait dépasser d'un pas, énorme si dt est grand.)
    //    x, y = coin HAUT-GAUCHE, donc le max est la largeur MOINS la taille du joueur.
    if (joueur.x > LARGEUR_FENETRE - joueur.taille) { joueur.x = LARGEUR_FENETRE - joueur.taille; }
    if (joueur.y > HAUTEUR_FENETRE - joueur.taille) { joueur.y = HAUTEUR_FENETRE - joueur.taille; }
    if (joueur.x < 0.f) { joueur.x = 0.f; }
    if (joueur.y < 0.f) { joueur.y = 0.f; }

    // --- Dessin ---
    SDL_SetRenderDrawColor(state->renderer, 30, 30, 60, 255);
    SDL_RenderClear(state->renderer);

    SDL_FRect rect{joueur.x, joueur.y, joueur.taille, joueur.taille};
    SDL_SetRenderDrawColor(state->renderer, 230, 80, 80, 255);
    SDL_RenderFillRect(state->renderer, &rect);

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
