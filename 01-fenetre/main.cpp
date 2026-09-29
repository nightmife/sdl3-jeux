// Leçon 1 : ouvrir une fenêtre avec l'API "callbacks" de SDL3.
//
// Pas de main() ici : c'est SDL qui l'écrit pour nous et qui appelle
// nos 4 fonctions au bon moment :
//   SDL_AppInit    -> une fois au démarrage
//   SDL_AppEvent   -> à chaque événement (clavier, souris, fermeture...)
//   SDL_AppIterate -> à chaque frame (logique + dessin)
//   SDL_AppQuit    -> une fois à la fin, même si Init a échoué

// Doit être défini AVANT d'inclure SDL_main.h, sinon SDL attend un main() classique
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

// Tout l'état du programme. SDL nous le repasse à chaque callback via
// un void*, ce qui évite les variables globales.
struct AppState {
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
};

SDL_AppResult SDL_AppInit(void** appstate, int /*argc*/, char* /*argv*/[])
{
    auto* state = new AppState{};
    *appstate = state;  // SDL garde ce pointeur et nous le rendra partout

    // Démarre le sous-système vidéo (fenêtres, rendu, clavier/souris).
    // En SDL3, les fonctions renvoient un bool : false = échec, et la raison
    // se récupère avec SDL_GetError() (à ne lire QUE juste après un échec).
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        // Le message dit QUELLE étape a raté, SDL_GetError() dit POURQUOI
        SDL_Log("SDL_Init a échoué : %s", SDL_GetError());
        // SDL_APP_FAILURE : SDL arrête tout proprement (et appelle quand même
        // SDL_AppQuit, qui libérera "state" : pas de fuite mémoire)
        return SDL_APP_FAILURE;
    }

    // Crée la fenêtre ET son renderer (l'objet qui dessine dedans) d'un coup.
    // On passe l'ADRESSE de nos deux pointeurs pour que SDL les remplisse.
    if (!SDL_CreateWindowAndRenderer("titre", 1280, 720, 0, &state->window, &state->renderer)) {
        SDL_Log("Création de la fenêtre impossible : %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Tout est prêt : SDL peut commencer à appeler SDL_AppIterate à chaque frame
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* /*appstate*/, SDL_Event* event)
{
    if (event->type == SDL_EVENT_QUIT)  // croix de la fenêtre, Alt+F4...
        return SDL_APP_SUCCESS;         // demande à SDL d'arrêter proprement

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    auto* state = static_cast<AppState*>(appstate);

    // Le rendu se fait toujours en 3 temps : effacer, dessiner, présenter
    SDL_SetRenderDrawColor(state->renderer, 30, 30, 60, 255);
    SDL_RenderClear(state->renderer);
    // (rien à dessiner pour l'instant)
    SDL_RenderPresent(state->renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult /*result*/)
{
    auto* state = static_cast<AppState*>(appstate);
    if (!state)
        return;

    // Ordre inverse de la création : le renderer dépend de la fenêtre
    if (state->renderer) SDL_DestroyRenderer(state->renderer);
    if (state->window)   SDL_DestroyWindow(state->window);
    delete state;
    // SDL_Quit() est appelé automatiquement après cette fonction
}
