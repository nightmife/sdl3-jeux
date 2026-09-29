// Leçon 6, étape A : plusieurs joueurs. Pour tester la logique SANS réseau :
// deux joueurs sur le même clavier (joueur 1 : ZQSD, joueur 2 : flèches).

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <string>

#include "jeu.h"

// Les images du jeu, chargées en mémoire vidéo (GPU)
struct Textures {
    SDL_Texture* joueur = nullptr;
    SDL_Texture* piece  = nullptr;
};

// Un texte déjà transformé en texture, avec le texte qu'il affiche.
// On ne le régénère que quand le texte change.
struct TexteCache {
    SDL_Texture* texture = nullptr;
    std::string  texte;  // vide : rien n'a encore été généré
};

// Nombre de joueurs de la partie locale
constexpr int NB_JOUEURS_LOCAUX = 2;

// Teinte de chaque joueur (multipliée aux couleurs du sprite)
constexpr SDL_Color COULEURS_JOUEURS[MAX_JOUEURS] = {
    {255, 255, 255, 255},  // joueur 1 : couleurs d'origine (rouge)
    {120, 200, 255, 255},  // joueur 2 : bleuté
    {140, 255, 140, 255},  // joueur 3 : verdâtre
    {255, 230, 120, 255},  // joueur 4 : jaunâtre
};

// Un son chargé en mémoire, et le "tuyau" qui l'envoie vers la carte son
struct Son {
    SDL_AudioStream* stream  = nullptr;  // relié au périphérique audio par défaut
    Uint8*           donnees = nullptr;  // échantillons bruts du fichier WAV
    Uint32           taille  = 0;        // en octets
};

// L'écran sur lequel on se trouve. Un seul à la fois : c'est une machine à états.
enum class Ecran {
    Menu,    // titre, attend Entrée
    Partie,  // on joue
    Pause,   // partie figée, affichée en fond
};

// Textes qui ne changent jamais : générés une seule fois à l'init
struct TextesFixes {
    SDL_Texture* titre = nullptr;
    SDL_Texture* aide  = nullptr;
    SDL_Texture* pause = nullptr;
};

struct AppState {
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    Textures      textures;
    TTF_Font*     police = nullptr;
    TexteCache    texteScore;
    Son           sonPiece;
    Ecran         ecran = Ecran::Menu;
    TextesFixes   textes;
    Jeu           jeu;
    Uint64        dernierTemps = 0;  // instant de la frame précédente, en nanosecondes
    float         accumulateur = 0;  // temps réel pas encore "consommé" par la logique, en s
};

// Traduit l'état du clavier SDL en intentions, pour chaque joueur local :
// joueur 1 sur ZQSD (WASD en QWERTY), joueur 2 sur les flèches
static std::vector<Entrees> lireEntrees()
{
    const bool* clavier = SDL_GetKeyboardState(nullptr);
    std::vector<Entrees> entrees(NB_JOUEURS_LOCAUX);

    entrees[0].haut   = clavier[SDL_SCANCODE_W];
    entrees[0].bas    = clavier[SDL_SCANCODE_S];
    entrees[0].gauche = clavier[SDL_SCANCODE_A];
    entrees[0].droite = clavier[SDL_SCANCODE_D];

    entrees[1].haut   = clavier[SDL_SCANCODE_UP];
    entrees[1].bas    = clavier[SDL_SCANCODE_DOWN];
    entrees[1].gauche = clavier[SDL_SCANCODE_LEFT];
    entrees[1].droite = clavier[SDL_SCANCODE_RIGHT];
    return entrees;
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

// Charge assets/<nom> (WAV) et ouvre un flux audio au format du fichier.
// En cas d'échec, le Son reste vide : le jeu continue, sans ce bruitage.
static void chargerSon(Son& son, const char* nom)
{
    const char* base = SDL_GetBasePath();
    std::string chemin = std::string(base ? base : "") + "assets/" + nom;

    SDL_AudioSpec format;  // fréquence, nombre de canaux, format des échantillons
    if (!SDL_LoadWAV(chemin.c_str(), &format, &son.donnees, &son.taille)) {
        SDL_Log("Impossible de charger %s : %s", chemin.c_str(), SDL_GetError());
        return;
    }

    // Flux vers la sortie audio par défaut. SDL convertit tout seul le format
    // du fichier vers celui de la carte son si besoin.
    son.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &format, nullptr, nullptr);
    if (!son.stream) {
        SDL_Log("Impossible d'ouvrir la sortie audio : %s", SDL_GetError());
        return;
    }
    SDL_ResumeAudioStreamDevice(son.stream);  // un flux ouvert démarre en pause
}

// Joue le son depuis le début (coupe la lecture précédente s'il y en a une)
static void jouerSon(Son& son)
{
    if (!son.stream) return;
    SDL_ClearAudioStream(son.stream);                        // vide ce qui restait à jouer
    SDL_PutAudioStreamData(son.stream, son.donnees, static_cast<int>(son.taille));
}

// Transforme un texte en texture (nullptr si pas de police ou en cas d'échec)
static SDL_Texture* creerTexte(SDL_Renderer* renderer, TTF_Font* police, const char* texte)
{
    if (!police) return nullptr;
    SDL_Surface* surface = TTF_RenderText_Blended(police, texte, 0, SDL_Color{255, 255, 255, 255});
    if (!surface) return nullptr;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    return texture;
}

// Dessine un texte centré horizontalement à la hauteur y.
// Plan B sans texture : la police de debug avec le même texte.
static void dessinerTexteCentre(SDL_Renderer* renderer, SDL_Texture* texture,
                                const char* secours, float y)
{
    if (texture) {
        float w = 0, h = 0;
        SDL_GetTextureSize(texture, &w, &h);
        SDL_FRect dst{(LARGEUR_MONDE - w) / 2.0f, y, w, h};
        SDL_RenderTexture(renderer, texture, nullptr, &dst);
    } else {
        // La police de debug fait 8 pixels par caractère (en ASCII)
        float w = 8.0f * static_cast<float>(SDL_strlen(secours));
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(renderer, (LARGEUR_MONDE - w) / 2.0f, y, secours);
    }
}

// Assombrit tout l'écran avec un voile noir semi-transparent
static void dessinerVoile(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);  // active la transparence
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);             // alpha 160/255
    SDL_RenderFillRect(renderer, nullptr);                      // nullptr = tout l'écran
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

// Texte des scores de tous les joueurs, par exemple "J1 : 3   J2 : 5"
static std::string texteScores(const Jeu& jeu)
{
    std::string texte;
    for (size_t i = 0; i < jeu.joueurs.size(); ++i) {
        if (i > 0) texte += "   ";
        texte += "J" + std::to_string(i + 1) + " : " + std::to_string(jeu.joueurs[i].score);
    }
    return texte;
}

// Met à jour la texture des scores si (et seulement si) le texte a changé
static void mettreAJourTexteScore(SDL_Renderer* renderer, TTF_Font* police,
                                  TexteCache& cache, const std::string& texte)
{
    // 1. Sorties anticipées : pas de police (dessiner() utilisera le plan B),
    //    ou texte inchangé (la texture actuelle est encore bonne : c'est le CACHE)
    if (police == nullptr) return;
    if (cache.texte == texte) return;
 
    // 2. Texte -> surface (image en RAM, dessinée lettre par lettre : coûteux,
    //    d'où le cache). Le 0 = "le texte se termine par '\0'".
    SDL_Surface *surface = TTF_RenderText_Blended(police, texte.c_str(), 0, SDL_Color{255, 255, 255, 255});
    
    if (surface == nullptr) {
        SDL_Log("Rendu du texte impossible: %s", SDL_GetError());
        return;
    }

    // 3. Surface -> texture (image sur le GPU). On RANGE le résultat : sans ça,
    //    la texture serait perdue et fuirait. La surface ne sert plus ensuite.
    SDL_Texture *nouvelle =  SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (nouvelle == nullptr) {
        SDL_Log("Création de la texture du texte impossible: %s", SDL_GetError());
        return;
    }
    
    // 4. Remplacement SÛR : on ne détruit l'ancienne texture qu'une fois la
    //    nouvelle créée avec succès (si ça échoue, l'ancienne reste affichée et
    //    on réessaiera à la frame suivante puisque la valeur n'est pas mise à jour).
    if (cache.texture) SDL_DestroyTexture(cache.texture);
    cache.texture = nouvelle;
    cache.texte = texte;
}

// Dessine l'état du jeu (sans Present : l'écran courant peut ajouter
// des choses par-dessus). Reçoit le jeu en const : dessiner ne modifie rien.
static void dessiner(SDL_Renderer* renderer, const Textures& textures,
                     const TexteCache& texteScore, const Jeu& jeu)
{
    SDL_SetRenderDrawColor(renderer, 30, 30, 60, 255);
    SDL_RenderClear(renderer);

    // Pièces puis joueur, avec leur texture
    // Couleur de SECOURS réglée une fois avant la boucle : elle ne sert que si
    // la texture manque (SDL_RenderTexture, lui, ignore la couleur de dessin).
    SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255);
    for (const Piece &p : jeu.pieces) {
        // dstrect = où et à quelle taille dessiner ; srcrect = nullptr = toute l'image.
        // Plan B si l'image n'a pas pu être chargée : un rectangle de couleur.
        SDL_FRect rect{p.x, p.y, p.taille, p.taille};
        if (textures.piece == nullptr) { SDL_RenderFillRect(renderer, &rect); }
        else { SDL_RenderTexture(renderer, textures.piece, nullptr, &rect); }
    }
    // Chaque joueur avec sa teinte (SDL_SetTextureColorMod multiplie les couleurs du sprite)
    for (size_t i = 0; i < jeu.joueurs.size(); ++i) {
        const Joueur& j = jeu.joueurs[i];
        const SDL_Color c = COULEURS_JOUEURS[i];
        SDL_FRect rect{j.x, j.y, j.taille, j.taille};
        if (textures.joueur == nullptr) {
            SDL_SetRenderDrawColor(renderer, c.r, c.g / 3, c.b / 3, 255);
            SDL_RenderFillRect(renderer, &rect);
        } else {
            SDL_SetTextureColorMod(textures.joueur, c.r, c.g, c.b);
            SDL_RenderTexture(renderer, textures.joueur, nullptr, &rect);
        }
    }
    if (textures.joueur) SDL_SetTextureColorMod(textures.joueur, 255, 255, 255);

    // Score : texture de texte si dispo, sinon la police de debug (plan B)
    if (texteScore.texture) {
        float w = 0, h = 0;
        SDL_GetTextureSize(texteScore.texture, &w, &h);  // taille réelle du texte rendu
        SDL_FRect dst{10, 10, w, h};
        SDL_RenderTexture(renderer, texteScore.texture, nullptr, &dst);
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(renderer, 10, 10, texteScores(jeu).c_str());
    }
}

SDL_AppResult SDL_AppInit(void** appstate, int /*argc*/, char* /*argv*/[])
{
    auto* state = new AppState{};
    *appstate = state;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init a échoué : %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Leçon 6 - Multi-joueurs local", static_cast<int>(LARGEUR_MONDE),
                                     static_cast<int>(HAUTEUR_MONDE), 0,
                                     &state->window, &state->renderer)) {
        SDL_Log("Création de la fenêtre impossible : %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetRenderVSync(state->renderer, 1);

    // Une image manquante n'empêche pas de jouer : dessiner() prévoit un plan B
    state->textures.joueur = chargerTexture(state->renderer, "joueur.png");
    state->textures.piece  = chargerTexture(state->renderer, "piece.png");

    chargerSon(state->sonPiece, "piece.wav");

    // SDL_ttf a son propre init. Comme pour les images : pas de police = plan B.
    if (!TTF_Init()) {
        SDL_Log("TTF_Init a échoué : %s", SDL_GetError());
    } else {
        const char* base = SDL_GetBasePath();
        std::string chemin = std::string(base ? base : "") + "assets/police.ttf";
        state->police = TTF_OpenFont(chemin.c_str(), 32.0f);  // taille en points
        if (!state->police)
            SDL_Log("Impossible de charger %s : %s", chemin.c_str(), SDL_GetError());
    }

    state->textes.titre = creerTexte(state->renderer, state->police, "CHASSE AUX PIÈCES");
    state->textes.aide  = creerTexte(state->renderer, state->police, "Entrée : jouer   Échap : quitter");
    state->textes.pause = creerTexte(state->renderer, state->police, "PAUSE  (P : reprendre, Retour arrière : menu)");

    // Une partie est préparée dès le départ, pour l'afficher derrière le menu
    initialiser(state->jeu, static_cast<unsigned>(SDL_GetPerformanceCounter()), NB_JOUEURS_LOCAUX);

    state->dernierTemps = SDL_GetTicksNS();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    auto* state = static_cast<AppState*>(appstate);

    if (event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;

    // Appui sur une touche. On ignore les répétitions automatiques du clavier :
    // sinon, rester appuyé sur P ferait clignoter la pause.
    if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
        SDL_Scancode touche = event->key.scancode;

        // Transitions entre écrans : on regarde D'ABORD où on est (le switch),
        // PUIS la touche. La même touche (Échap) fait donc des choses différentes
        // selon l'écran. || seulement pour des touches qui mènent au MÊME écran.
        switch (state->ecran) {
        case Ecran::Menu:
            if (touche == SDL_SCANCODE_RETURN) {
                initialiser(state->jeu, static_cast<unsigned>(SDL_GetPerformanceCounter()), NB_JOUEURS_LOCAUX);
                state->ecran = Ecran::Partie;
            }
            if (touche == SDL_SCANCODE_ESCAPE) return SDL_APP_SUCCESS;
            break;
        case Ecran::Partie:
            if (touche == SDL_SCANCODE_P || touche == SDL_SCANCODE_ESCAPE) state->ecran = Ecran::Pause;
            break;
        case Ecran::Pause:
            if (touche == SDL_SCANCODE_P || touche == SDL_SCANCODE_ESCAPE) state->ecran = Ecran::Partie;
            else if (touche == SDL_SCANCODE_BACKSPACE) state->ecran = Ecran::Menu;
            break;
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    auto* state = static_cast<AppState*>(appstate);

    // DELTA TIME : temps écoulé depuis la frame précédente, en secondes.
    // - Horloge lue UNE fois (sinon le temps entre deux lectures serait perdu).
    // - Uint64 et pas float : un float arrondirait ce grand nombre de ns.
    // - Soustraction en entier (exacte), PUIS conversion en float, PUIS division
    //   en float (une division entière donnerait 0).
    Uint64 maintenant = SDL_GetTicksNS();
    float dt = static_cast<float>(maintenant - state->dernierTemps) / SDL_NS_PER_SECOND;
    state->dernierTemps = maintenant;

    // Le jeu n'avance QUE pendant la partie. Le dt est quand même calculé à
    // chaque frame, pour ne pas avoir un énorme dt au moment de reprendre.
    if (state->ecran == Ecran::Partie) {
        const std::vector<Entrees> entrees = lireEntrees();
        const int scoreAvant = scoreTotal(state->jeu);

        // Avancer la logique par pas fixes
        // Plafond anti « spirale de la mort » : après un gel de 2 s, on ne veut pas
        // enchaîner 120 pas d'un coup (ce qui ferait encore plus ramer).
        if (dt > 0.25f) dt = 0.25f;

        // PAS DE TEMPS FIXE (la "tirelire") : on accumule le temps réel, et on le
        // dépense par pas identiques de PAS_FIXE (1/60 s). Tous les écrans (60 Hz,
        // 144 Hz, PC lent) font donc exactement les mêmes calculs : déterminisme.
        state->accumulateur += dt;

        while (state->accumulateur >= PAS_FIXE) {
            mettreAJour(state->jeu, entrees, PAS_FIXE);
            state->accumulateur -= PAS_FIXE;
        }

        // Bruitage si au moins une pièce a été ramassée pendant cette mise à jour
        if (scoreAvant != scoreTotal(state->jeu)) jouerSon(state->sonPiece);
    }

    mettreAJourTexteScore(state->renderer, state->police, state->texteScore, texteScores(state->jeu));

    // Le jeu est toujours dessiné ; menu et pause s'affichent par-dessus
    dessiner(state->renderer, state->textures, state->texteScore, state->jeu);
    switch (state->ecran) {
    case Ecran::Menu:
        dessinerVoile(state->renderer);
        dessinerTexteCentre(state->renderer, state->textes.titre, "CHASSE AUX PIECES", 280);
        dessinerTexteCentre(state->renderer, state->textes.aide,
                            "Entree : jouer   Echap : quitter", 360);
        break;
    case Ecran::Pause:
        dessinerVoile(state->renderer);
        dessinerTexteCentre(state->renderer, state->textes.pause,
                            "PAUSE  (P : reprendre, Retour arriere : menu)", 330);
        break;
    case Ecran::Partie:
        break;
    }

    SDL_RenderPresent(state->renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult /*result*/)
{
    auto* state = static_cast<AppState*>(appstate);
    if (!state)
        return;

    // Les textures appartiennent au renderer : on les détruit AVANT lui
    if (state->texteScore.texture) SDL_DestroyTexture(state->texteScore.texture);
    if (state->textes.titre) SDL_DestroyTexture(state->textes.titre);
    if (state->textes.aide)  SDL_DestroyTexture(state->textes.aide);
    if (state->textes.pause) SDL_DestroyTexture(state->textes.pause);
    if (state->textures.piece)  SDL_DestroyTexture(state->textures.piece);
    if (state->textures.joueur) SDL_DestroyTexture(state->textures.joueur);
    if (state->renderer) SDL_DestroyRenderer(state->renderer);
    if (state->window)   SDL_DestroyWindow(state->window);

    // Le flux d'abord (il lit peut-être encore les données), puis les données
    if (state->sonPiece.stream) SDL_DestroyAudioStream(state->sonPiece.stream);
    SDL_free(state->sonPiece.donnees);  // alloué par SDL_LoadWAV : libéré avec SDL_free

    if (state->police) TTF_CloseFont(state->police);
    TTF_Quit();  // sans danger même si TTF_Init a échoué
    delete state;
}
