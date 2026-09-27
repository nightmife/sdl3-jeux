// Leçon 4 : textures. On remplace les carrés par des images PNG (SDL_image).
// jeu.h / jeu.cpp n'ont PAS changé : seul l'affichage évolue.

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

#include <string>

#include "jeu.h"

// Les images du jeu, chargées en mémoire vidéo (GPU)
struct Textures {
    SDL_Texture* joueur = nullptr;
    SDL_Texture* piece  = nullptr;
};

struct AppState {
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    Textures      textures;
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

// Charge assets/<nom> depuis le dossier de l'exécutable.
// Renvoie nullptr (et affiche pourquoi) si le chargement échoue.
static SDL_Texture* chargerTexture(SDL_Renderer* renderer, const char* nom)
{
    // Dossier de l'exécutable (avec '/' final), et non le dossier courant du terminal
    const char* base = SDL_GetBasePath();
    std::string chemin = std::string(base ? base : "") + "assets/" + nom;

    SDL_Texture* texture = IMG_LoadTexture(renderer, chemin.c_str());
    if (!texture) {
        SDL_Log("Impossible de charger %s : %s", chemin.c_str(), SDL_GetError());
        return nullptr;
    }

    // Pixel art : agrandir sans flou (chaque pixel devient un gros carré net)
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    return texture;
}

// Dessine l'état du jeu. Reçoit le jeu en const : dessiner ne modifie rien.
static void dessiner(SDL_Renderer* renderer, const Textures& textures, const Jeu& jeu)
{
    SDL_SetRenderDrawColor(renderer, 30, 30, 60, 255);
    SDL_RenderClear(renderer);

    // Pièces puis joueur, avec leur texture
    SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255);
    for (const Piece &p : jeu.pieces) {
        SDL_FRect rect{p.x, p.y, p.taille, p.taille};
        if (textures.piece == nullptr) { SDL_RenderFillRect(renderer, &rect); }
        else { SDL_RenderTexture(renderer, textures.piece, nullptr, &rect); }
    }
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    const Joueur &j = jeu.joueur;
    SDL_FRect rect{j.x, j.y, j.taille, j.taille};
    if (textures.joueur == nullptr) { SDL_RenderFillRect(renderer, &rect); }
    else { SDL_RenderTexture(renderer, textures.joueur, nullptr, &rect); }

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

    if (!SDL_CreateWindowAndRenderer("Leçon 4 - Textures", static_cast<int>(LARGEUR_MONDE),
                                     static_cast<int>(HAUTEUR_MONDE), 0,
                                     &state->window, &state->renderer)) {
        SDL_Log("Création de la fenêtre impossible : %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetRenderVSync(state->renderer, 1);

    // Une image manquante n'empêche pas de jouer : dessiner() prévoit un plan B
    state->textures.joueur = chargerTexture(state->renderer, "joueur.png");
    state->textures.piece  = chargerTexture(state->renderer, "piece.png");

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
    dessiner(state->renderer, state->textures, state->jeu);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult /*result*/)
{
    auto* state = static_cast<AppState*>(appstate);
    if (!state)
        return;

    // Les textures appartiennent au renderer : on les détruit AVANT lui
    if (state->textures.piece)  SDL_DestroyTexture(state->textures.piece);
    if (state->textures.joueur) SDL_DestroyTexture(state->textures.joueur);
    if (state->renderer) SDL_DestroyRenderer(state->renderer);
    if (state->window)   SDL_DestroyWindow(state->window);
    delete state;
}
