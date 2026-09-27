// Leçon 3 (fin) : séparer la logique du jeu (jeu.h / jeu.cpp) du code SDL.
// Ce fichier ne fait plus que trois choses : lire le clavier, appeler la
// logique, dessiner. Il ne contient AUCUNE règle du jeu.

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "jeu.h"

struct AppState {
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    Jeu           jeu;
    Uint64        dernierTemps = 0;  // instant de la frame précédente, en nanosecondes
};

// Traduit l'état du clavier SDL en intentions pour la logique du jeu
static Entrees lireEntrees()
{
    const bool* clavier = SDL_GetKeyboardState(nullptr);
    Entrees e;
    e.haut = clavier[SDL_SCANCODE_W];
    e.bas = clavier[SDL_SCANCODE_S];
    e.gauche = clavier[SDL_SCANCODE_A];
    e.droite = clavier[SDL_SCANCODE_D];
    return e;
}

// Dessine l'état du jeu. Reçoit le jeu en const : dessiner ne modifie rien.
static void dessiner(SDL_Renderer* renderer, const Jeu& jeu)
{
    SDL_SetRenderDrawColor(renderer, 30, 30, 60, 255);
    SDL_RenderClear(renderer);

    // Pièces en jaune
    SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255);
    for (const Piece& p : jeu.pieces) {
        SDL_FRect rect{p.x, p.y, p.taille, p.taille};
        SDL_RenderFillRect(renderer, &rect);
    }

    // Joueur en rouge
    const Joueur& j = jeu.joueur;
    SDL_FRect rect{j.x, j.y, j.taille, j.taille};
    SDL_SetRenderDrawColor(renderer, 230, 80, 80, 255);
    SDL_RenderFillRect(renderer, &rect);

    // Score
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugTextFormat(renderer, 10, 10, "Score : %d", jeu.score);

    SDL_RenderPresent(renderer);
}

SDL_AppResult SDL_AppInit(void** appstate, int /*argc*/, char* /*argv*/[])
{
    auto* state = new AppState{};
    *appstate = state;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init a échoué : %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Leçon 3 - Séparation", static_cast<int>(LARGEUR_MONDE),
                                     static_cast<int>(HAUTEUR_MONDE), 0,
                                     &state->window, &state->renderer)) {
        SDL_Log("Création de la fenêtre impossible : %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetRenderVSync(state->renderer, 1);

    // Graine différente à chaque lancement : le compteur haute précision
    initialiser(state->jeu, static_cast<unsigned>(SDL_GetPerformanceCounter()));

    state->dernierTemps = SDL_GetTicksNS();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* /*appstate*/, SDL_Event* event)
{
    if (event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;

    if (event->type == SDL_EVENT_KEY_DOWN && event->key.scancode == SDL_SCANCODE_ESCAPE)
        return SDL_APP_SUCCESS;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    auto* state = static_cast<AppState*>(appstate);

    Uint64 maintenant = SDL_GetTicksNS();
    float dt = static_cast<float>(maintenant - state->dernierTemps) / SDL_NS_PER_SECOND;
    state->dernierTemps = maintenant;

    // Les 3 temps d'une frame : entrées -> logique -> dessin
    Entrees entrees = lireEntrees();
    mettreAJour(state->jeu, entrees, dt);
    dessiner(state->renderer, state->jeu);

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
