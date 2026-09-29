// Leçon 6, étape C : le jeu EN LIGNE.
// Un joueur héberge une salle, les autres la rejoignent avec son adresse
// (127.0.0.1 pour tester sur le même PC, ou l'adresse Tailscale / publique).
//
// Étape C1 : menu, saisie de l'adresse, salle d'attente, connexion non bloquante.

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_net/SDL_net.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <string>

#include "affichage.h"
#include "jeu.h"
#include "session.h"

// Un son chargé en mémoire, et le "tuyau" qui l'envoie vers la carte son
struct Son {
    SDL_AudioStream* stream  = nullptr;
    Uint8*           donnees = nullptr;
    Uint32           taille  = 0;
};

enum class Ecran {
    Menu,              // H : héberger, R : rejoindre
    SaisieAdresse,     // le client tape l'adresse de l'hôte
    ConnexionClient,   // le client se connecte / attend que l'hôte lance
    SalleAttenteHote,  // l'hôte voit les joueurs arriver
    Partie,
};

// Textes qui ne changent jamais : générés une seule fois à l'init
struct TextesFixes {
    SDL_Texture* titre = nullptr;
    SDL_Texture* aide  = nullptr;
};

struct AppState {
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    Textures      textures;
    TTF_Font*     police = nullptr;
    TexteCache    texteScore;
    TexteCache    ligne1;  // textes des écrans (changent selon la situation)
    TexteCache    ligne2;
    TextesFixes   textes;
    Son           sonPiece;
    Ecran         ecran = Ecran::Menu;
    Jeu           jeu;
    Uint64        dernierTemps = 0;
    float         accumulateur = 0;

    // Réseau
    bool          estHote = false;
    Hote          hote;
    Client        client;
    std::string   adresseSaisie = "127.0.0.1";
};

// Entrées du joueur de CE PC : ZQSD ou flèches, au choix
static Entrees lireEntrees()
{
    const bool* clavier = SDL_GetKeyboardState(nullptr);
    Entrees e;
    e.haut   = clavier[SDL_SCANCODE_W] || clavier[SDL_SCANCODE_UP];
    e.bas    = clavier[SDL_SCANCODE_S] || clavier[SDL_SCANCODE_DOWN];
    e.gauche = clavier[SDL_SCANCODE_A] || clavier[SDL_SCANCODE_LEFT];
    e.droite = clavier[SDL_SCANCODE_D] || clavier[SDL_SCANCODE_RIGHT];
    return e;
}

static void chargerSon(Son& son, const char* nom)
{
    const char* base = SDL_GetBasePath();
    std::string chemin = std::string(base ? base : "") + "assets/" + nom;
    SDL_AudioSpec format;
    if (!SDL_LoadWAV(chemin.c_str(), &format, &son.donnees, &son.taille)) {
        SDL_Log("Impossible de charger %s : %s", chemin.c_str(), SDL_GetError());
        return;
    }
    son.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &format, nullptr, nullptr);
    if (!son.stream) {
        SDL_Log("Impossible d'ouvrir la sortie audio : %s", SDL_GetError());
        return;
    }
    SDL_ResumeAudioStreamDevice(son.stream);
}

static void jouerSon(Son& son)
{
    if (!son.stream) return;
    SDL_ClearAudioStream(son.stream);
    SDL_PutAudioStreamData(son.stream, son.donnees, static_cast<int>(son.taille));
}

// Revenir au menu en coupant proprement le réseau
static void retourMenu(AppState* state)
{
    fermerHote(state->hote);
    fermerClient(state->client);
    SDL_StopTextInput(state->window);
    state->ecran = Ecran::Menu;
}

SDL_AppResult SDL_AppInit(void** appstate, int /*argc*/, char* /*argv*/[])
{
    auto* state = new AppState{};
    *appstate = state;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init a échoué : %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!NET_Init()) {
        SDL_Log("NET_Init a échoué : %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Chasse aux pièces - en ligne", static_cast<int>(LARGEUR_MONDE),
                                     static_cast<int>(HAUTEUR_MONDE), 0,
                                     &state->window, &state->renderer)) {
        SDL_Log("Création de la fenêtre impossible : %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderVSync(state->renderer, 1);

    state->textures.joueur = chargerTexture(state->renderer, "joueur.png");
    state->textures.piece  = chargerTexture(state->renderer, "piece.png");
    chargerSon(state->sonPiece, "piece.wav");

    if (!TTF_Init()) {
        SDL_Log("TTF_Init a échoué : %s", SDL_GetError());
    } else {
        const char* base = SDL_GetBasePath();
        std::string chemin = std::string(base ? base : "") + "assets/police.ttf";
        state->police = TTF_OpenFont(chemin.c_str(), 32.0f);
        if (!state->police)
            SDL_Log("Impossible de charger %s : %s", chemin.c_str(), SDL_GetError());
    }

    state->textes.titre = creerTexte(state->renderer, state->police, "CHASSE AUX PIÈCES");
    state->textes.aide  = creerTexte(state->renderer, state->police,
                                     "H : héberger   R : rejoindre   Échap : quitter");

    // Une partie à 1 joueur pour décorer le fond du menu
    initialiser(state->jeu, static_cast<unsigned>(SDL_GetPerformanceCounter()), 1);

    state->dernierTemps = SDL_GetTicksNS();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    auto* state = static_cast<AppState*>(appstate);

    if (event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;

    // Saisie de texte (l'adresse). SDL envoie les caractères tapés en UTF-8,
    // en tenant compte de la disposition du clavier (AZERTY...).
    if (event->type == SDL_EVENT_TEXT_INPUT && state->ecran == Ecran::SaisieAdresse) {
        if (state->adresseSaisie.size() < 60) state->adresseSaisie += event->text.text;
        return SDL_APP_CONTINUE;
    }

    if (event->type != SDL_EVENT_KEY_DOWN) return SDL_APP_CONTINUE;
    const SDL_Scancode touche = event->key.scancode;

    // Effacer dans la saisie : on autorise la répétition (rester appuyé efface vite)
    if (state->ecran == Ecran::SaisieAdresse && touche == SDL_SCANCODE_BACKSPACE) {
        if (!state->adresseSaisie.empty()) state->adresseSaisie.pop_back();
        return SDL_APP_CONTINUE;
    }
    if (event->key.repeat) return SDL_APP_CONTINUE;

    switch (state->ecran) {
    case Ecran::Menu:
        if (touche == SDL_SCANCODE_ESCAPE) return SDL_APP_SUCCESS;
        if (event->key.key == SDLK_H) {  // keycode : la LETTRE H, quelle que soit la disposition
            if (demarrerHote(state->hote)) {
                state->estHote = true;
                state->ecran = Ecran::SalleAttenteHote;
            }
        }
        if (event->key.key == SDLK_R) {
            state->estHote = false;
            state->ecran = Ecran::SaisieAdresse;
            SDL_StartTextInput(state->window);  // active la réception des SDL_EVENT_TEXT_INPUT
        }
        break;

    case Ecran::SaisieAdresse:
        if (touche == SDL_SCANCODE_ESCAPE) retourMenu(state);
        if (touche == SDL_SCANCODE_RETURN && !state->adresseSaisie.empty()) {
            SDL_StopTextInput(state->window);
            demarrerClient(state->client, state->adresseSaisie.c_str());
            state->ecran = Ecran::ConnexionClient;
        }
        break;

    case Ecran::ConnexionClient:
        if (touche == SDL_SCANCODE_ESCAPE) retourMenu(state);
        break;

    case Ecran::SalleAttenteHote:
        if (touche == SDL_SCANCODE_ESCAPE) retourMenu(state);
        if (touche == SDL_SCANCODE_RETURN) {
            // L'hôte + chaque joueur connecté
            const int nb = 1 + static_cast<int>(state->hote.joueurs.size());
            initialiser(state->jeu, static_cast<unsigned>(SDL_GetPerformanceCounter()), nb);
            state->accumulateur = 0;
            state->ecran = Ecran::Partie;
        }
        break;

    case Ecran::Partie:
        if (touche == SDL_SCANCODE_ESCAPE) retourMenu(state);
        break;
    }
    return SDL_APP_CONTINUE;
}

// Textes des deux lignes d'information, selon l'écran et l'état du réseau
static void textesEcran(const AppState* state, std::string& l1, std::string& l2)
{
    switch (state->ecran) {
    case Ecran::SaisieAdresse:
        l1 = "Adresse de l'hôte : " + state->adresseSaisie + "_";
        l2 = "Entrée : se connecter   Échap : retour";
        break;
    case Ecran::ConnexionClient:
        switch (state->client.etat) {
        case EtatClient::Resolution: l1 = "Recherche de " + state->adresseSaisie + "..."; break;
        case EtatClient::Connexion:  l1 = "Connexion à " + state->adresseSaisie + "..."; break;
        case EtatClient::Connecte:   l1 = "Connecté ! En attente du lancement par l'hôte"; break;
        case EtatClient::Echec:      l1 = "Échec : " + state->client.erreur; break;
        case EtatClient::Inactif:    l1 = ""; break;
        }
        l2 = "Échap : retour au menu";
        break;
    case Ecran::SalleAttenteHote:
        l1 = "Salle ouverte (port " + std::to_string(PORT) + ") - joueurs : "
           + std::to_string(1 + state->hote.joueurs.size()) + " / " + std::to_string(MAX_JOUEURS);
        l2 = "Entrée : lancer la partie   Échap : fermer la salle";
        break;
    default:
        l1 = l2 = "";
        break;
    }
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    auto* state = static_cast<AppState*>(appstate);

    const Uint64 maintenant = SDL_GetTicksNS();
    float dt = static_cast<float>(maintenant - state->dernierTemps) / SDL_NS_PER_SECOND;
    state->dernierTemps = maintenant;
    if (dt > 0.25f) dt = 0.25f;

    // --- Réseau : à chaque frame, sans jamais attendre ---
    if (state->ecran == Ecran::SalleAttenteHote)
        accepterNouveauxJoueurs(state->hote);
    if (state->ecran == Ecran::ConnexionClient)
        avancerClient(state->client);

    // --- Logique (C1 : seul l'hôte joue, les joueurs distants ne bougent pas encore) ---
    if (state->ecran == Ecran::Partie && state->estHote) {
        std::vector<Entrees> entrees(state->jeu.joueurs.size());
        entrees[0] = lireEntrees();  // l'hôte est le joueur 1
        const int scoreAvant = scoreTotal(state->jeu);

        state->accumulateur += dt;
        while (state->accumulateur >= PAS_FIXE) {
            mettreAJour(state->jeu, entrees, PAS_FIXE);
            state->accumulateur -= PAS_FIXE;
        }
        if (scoreAvant != scoreTotal(state->jeu)) jouerSon(state->sonPiece);
    }

    // --- Dessin ---
    mettreAJourTexte(state->renderer, state->police, state->texteScore, texteScores(state->jeu));
    dessiner(state->renderer, state->textures, state->texteScore, state->jeu);

    if (state->ecran != Ecran::Partie) {
        dessinerVoile(state->renderer);
        dessinerTexteCentre(state->renderer, state->textes.titre, "CHASSE AUX PIECES", 230);

        std::string l1, l2;
        textesEcran(state, l1, l2);
        if (state->ecran == Ecran::Menu) {
            dessinerTexteCentre(state->renderer, state->textes.aide,
                                "H : heberger   R : rejoindre   Echap : quitter", 330);
        } else {
            mettreAJourTexte(state->renderer, state->police, state->ligne1, l1);
            mettreAJourTexte(state->renderer, state->police, state->ligne2, l2);
            dessinerTexteCentre(state->renderer, state->ligne1.texture, l1.c_str(), 330);
            dessinerTexteCentre(state->renderer, state->ligne2.texture, l2.c_str(), 390);
        }
    }

    SDL_RenderPresent(state->renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult /*result*/)
{
    auto* state = static_cast<AppState*>(appstate);
    if (!state)
        return;

    fermerHote(state->hote);
    fermerClient(state->client);

    for (TexteCache* c : {&state->texteScore, &state->ligne1, &state->ligne2})
        if (c->texture) SDL_DestroyTexture(c->texture);
    if (state->textes.titre) SDL_DestroyTexture(state->textes.titre);
    if (state->textes.aide)  SDL_DestroyTexture(state->textes.aide);
    if (state->textures.piece)  SDL_DestroyTexture(state->textures.piece);
    if (state->textures.joueur) SDL_DestroyTexture(state->textures.joueur);
    if (state->renderer) SDL_DestroyRenderer(state->renderer);
    if (state->window)   SDL_DestroyWindow(state->window);

    if (state->sonPiece.stream) SDL_DestroyAudioStream(state->sonPiece.stream);
    SDL_free(state->sonPiece.donnees);

    if (state->police) TTF_CloseFont(state->police);
    TTF_Quit();
    NET_Quit();
    delete state;
}
