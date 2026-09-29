// Leçon 1b : le même programme que 01-fenetre, mais avec l'approche
// classique. Ici c'est NOUS qui écrivons main() et la boucle de jeu.
//
// Correspondance avec la version callbacks :
//   début de main()          <-> SDL_AppInit
//   while (SDL_PollEvent...) <-> SDL_AppEvent
//   dessiner()               <-> SDL_AppIterate
//   fin de main()            <-> SDL_AppQuit

// Pas de SDL_MAIN_USE_CALLBACKS : SDL s'attend à trouver notre main().
// On inclut quand même SDL_main.h : sur certaines plateformes (Windows, iOS...)
// il fait la plomberie nécessaire autour de main().
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static void dessiner(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 30, 30, 60, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

int main(int /*argc*/, char* /*argv*/[])
{
    // Plus besoin de AppState ni de void** : de simples variables locales
    // suffisent, car tout vit dans la même fonction.
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init a échoué : %s", SDL_GetError());
        return 1;
    }

    if (!SDL_CreateWindowAndRenderer("Version classique", 1280, 720, 0, &window, &renderer)) {
        SDL_Log("Création de la fenêtre impossible : %s", SDL_GetError());
        SDL_Quit();  // personne ne le fera à notre place, cette fois
        return 1;
    }

    // LA boucle de jeu : un tour = une frame.
    // "running" passe à false quand on veut quitter ; on finit alors le tour
    // en cours puis on sort (un break ne sortirait que de la boucle intérieure).
    bool running = true;
    while (running) {
        // 1. Vider TOUTE la file d'événements : entre deux frames il peut y en
        //    avoir 0, 1 ou 50. SDL_PollEvent en retire un à chaque appel et
        //    renvoie false quand la file est vide (sans jamais attendre).
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) { running = false; }  // croix, Alt+F4...
        }

        // 2. Dessiner la frame (équivalent de SDL_AppIterate)
        dessiner(renderer);
    }

    // Nettoyage : c'est à nous de tout faire, SDL_Quit() compris
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
