// Logique du jeu : AUCUNE dépendance à SDL.
// Ce fichier pourrait tourner tel quel sur un serveur sans écran.
#pragma once

#include <random>
#include <vector>

constexpr float LARGEUR_MONDE = 1280.0f;
constexpr float HAUTEUR_MONDE = 720.0f;
constexpr float VITESSE       = 300.0f;  // en pixels par seconde
constexpr int   NB_PIECES     = 10;

// Rectangle aligné sur les axes (AABB)
struct Rect {
    float x, y;  // coin haut-gauche
    float w, h;  // largeur, hauteur
};

struct Joueur {
    float x = 0.0f;
    float y = 0.0f;
    float taille = 50.0f;

    Rect hitbox() const { return Rect{x, y, taille, taille}; }
};

struct Piece {
    float x = 0.0f;
    float y = 0.0f;
    float taille = 20.0f;

    Rect hitbox() const { return Rect{x, y, taille, taille}; }
};

// Ce que le joueur VEUT faire pendant cette frame.
// Seules les intentions brutes : c'est la logique qui décide du mouvement réel.
struct Entrees {
    bool haut   = false;
    bool bas    = false;
    bool gauche = false;
    bool droite = false;
};

// Tout l'état du jeu
struct Jeu {
    Joueur             joueur;
    std::vector<Piece> pieces;
    int                score = 0;
    std::mt19937       rng;  // générateur aléatoire propre au jeu
};

void initialiser(Jeu& jeu, unsigned graine);
void mettreAJour(Jeu& jeu, const Entrees& entrees, float dt);
