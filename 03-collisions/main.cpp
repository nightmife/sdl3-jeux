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

// Vrai si les deux rectangles se chevauchent
static bool collision(const Rect& a, const Rect& b)
{
    return (a.x <= b.x + b.w && a.x + a.w >= b.x && a.y <= b.y + b.h && a.y + a.h >= b.y); 
}

struct AppState {
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    Joueur        joueur;
    std::vector<Piece> pieces;
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

    // Pièces à des positions aléatoires. SDL_randf() renvoie un float dans [0, 1[,
    // qu'on étire sur la zone où la pièce reste entièrement visible.
    for (int i = 0; i < NB_PIECES; ++i) {
        Piece p;
        p.x = SDL_randf() * (LARGEUR_FENETRE - p.taille);
        p.y = SDL_randf() * (HAUTEUR_FENETRE - p.taille);
        state->pieces.push_back(p);  // ajoute une copie de p à la fin du vector
    }

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
    
    Uint64 maintenant = SDL_GetTicksNS();
    float dt = static_cast<float>(maintenant - state->dernierTemps) / SDL_NS_PER_SECOND;
    state->dernierTemps = maintenant;
    
    // Déplacement : direction (dx, dy) normalisée, puis vitesse * dt
    float dx = 0.f, dy = 0.f;

    if (clavier[SDL_SCANCODE_D]) { dx += 1; } 
    if (clavier[SDL_SCANCODE_A]) { dx -= 1; }
    if (clavier[SDL_SCANCODE_W]) { dy -= 1; }
    if (clavier[SDL_SCANCODE_S]) { dy += 1; }

    float norme = SDL_sqrtf((dx * dx) + (dy * dy));
    if (norme > 0) {
        dx /= norme;
        dy /= norme;
    }

    joueur.x += VITESSE * dx * dt;
    joueur.y += VITESSE * dy * dt;

    // Garder le joueur dans la fenêtre
    if (joueur.x > LARGEUR_FENETRE - joueur.taille) { joueur.x = LARGEUR_FENETRE - joueur.taille; }
    if (joueur.y > HAUTEUR_FENETRE - joueur.taille) { joueur.y = HAUTEUR_FENETRE - joueur.taille; }
    if (joueur.x < 0.f) { joueur.x = 0.f; }
    if (joueur.y < 0.f) { joueur.y = 0.f; }

    // --- Dessin ---
    SDL_SetRenderDrawColor(state->renderer, 30, 30, 60, 255);
    SDL_RenderClear(state->renderer);

    // Pièces en jaune, en vert si le joueur les touche (test temporaire)
    Rect rJoueur{joueur.x, joueur.y, joueur.taille, joueur.taille};
    for (const Piece &p : state->pieces) {
        Rect rPiece{p.x, p.y, p.taille, p.taille};
        if (collision(rJoueur, rPiece))
            SDL_SetRenderDrawColor(state->renderer, 80, 220, 80, 255);
        else
            SDL_SetRenderDrawColor(state->renderer, 255, 200, 0, 255);

        SDL_FRect rect{p.x, p.y, p.taille, p.taille};
        SDL_RenderFillRect(state->renderer, &rect);
    }

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
