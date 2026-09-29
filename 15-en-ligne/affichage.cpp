#include "affichage.h"

#include <SDL3_image/SDL_image.h>

// Teinte de chaque joueur (multipliée aux couleurs du sprite)
static constexpr SDL_Color COULEURS_JOUEURS[MAX_JOUEURS] = {
    {255, 255, 255, 255},  // joueur 1 : couleurs d'origine (rouge)
    {120, 200, 255, 255},  // joueur 2 : bleuté
    {140, 255, 140, 255},  // joueur 3 : verdâtre
    {255, 230, 120, 255},  // joueur 4 : jaunâtre
};

// Charge assets/<nom> depuis le dossier de l'exécutable.
// Renvoie nullptr (et affiche pourquoi) si le chargement échoue.
SDL_Texture* chargerTexture(SDL_Renderer* renderer, const char* nom)
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

// Transforme un texte en texture (nullptr si pas de police ou en cas d'échec)
SDL_Texture* creerTexte(SDL_Renderer* renderer, TTF_Font* police, const char* texte)
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
void dessinerTexteCentre(SDL_Renderer* renderer, SDL_Texture* texture,
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
void dessinerVoile(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);  // active la transparence
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);             // alpha 160/255
    SDL_RenderFillRect(renderer, nullptr);                      // nullptr = tout l'écran
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

// Texte des scores de tous les joueurs, par exemple "J1 : 3   J2 : 5"
std::string texteScores(const Jeu& jeu)
{
    std::string texte;
    for (size_t i = 0; i < jeu.joueurs.size(); ++i) {
        if (i > 0) texte += "   ";
        texte += "J" + std::to_string(i + 1) + " : " + std::to_string(jeu.joueurs[i].score);
    }
    return texte;
}

// Met à jour la texture du cache si (et seulement si) le texte a changé
void mettreAJourTexte(SDL_Renderer* renderer, TTF_Font* police,
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
void dessiner(SDL_Renderer* renderer, const Textures& textures,
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

