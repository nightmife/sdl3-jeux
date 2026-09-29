// Tout ce qui dessine : textures, textes, écrans. Déplacé depuis main.cpp
// (leçons 4 et 5) pour que main.cpp reste lisible.
#pragma once

#include <SDL3/SDL.h>
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

// Charge assets/<nom> depuis le dossier de l'exécutable (nullptr en cas d'échec)
SDL_Texture* chargerTexture(SDL_Renderer* renderer, const char* nom);

// Transforme un texte en texture (nullptr si pas de police ou en cas d'échec)
SDL_Texture* creerTexte(SDL_Renderer* renderer, TTF_Font* police, const char* texte);

// Met à jour la texture du cache si (et seulement si) le texte a changé
void mettreAJourTexte(SDL_Renderer* renderer, TTF_Font* police,
                      TexteCache& cache, const std::string& texte);

// Dessine un texte centré horizontalement à la hauteur y (plan B : police de debug)
void dessinerTexteCentre(SDL_Renderer* renderer, SDL_Texture* texture,
                         const char* secours, float y);

// Assombrit tout l'écran avec un voile noir semi-transparent
void dessinerVoile(SDL_Renderer* renderer);

// Texte des scores de tous les joueurs, par exemple "J1 : 3   J2 : 5"
std::string texteScores(const Jeu& jeu);

// Dessine l'état du jeu (sans Present)
void dessiner(SDL_Renderer* renderer, const Textures& textures,
              const TexteCache& texteScore, const Jeu& jeu);
