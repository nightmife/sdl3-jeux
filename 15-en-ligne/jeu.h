// Logique du jeu : AUCUNE dépendance à SDL.
// Version multi-joueurs : chaque joueur a sa position, son score et ses entrées.
#pragma once

#include <random>
#include <vector>

constexpr float LARGEUR_MONDE = 1280.0f;
constexpr float HAUTEUR_MONDE = 720.0f;
constexpr float VITESSE       = 300.0f;  // en pixels par seconde
constexpr int   NB_PIECES     = 10;
constexpr int   MAX_JOUEURS   = 4;

// Pas de temps fixe de la logique : 60 mises à jour par seconde, toujours
// identiques, quel que soit l'écran. C'est la condition du déterminisme.
constexpr float PAS_FIXE = 1.0f / 60.0f;

// Rectangle aligné sur les axes (AABB)
struct Rect {
    float x, y;  // coin haut-gauche
    float w, h;  // largeur, hauteur
};

struct Joueur {
    float x = 0.0f;
    float y = 0.0f;
    float taille = 50.0f;
    int   score  = 0;  // chaque joueur a maintenant son propre score
    bool  actif  = true;  // false = le joueur a quitté : sa place reste réservée
                          // (les numéros des autres ne changent pas), mais il
                          // ne bouge plus, ne ramasse plus et n'est plus dessiné

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
    std::vector<Joueur> joueurs;  // joueurs[0] = joueur 1, joueurs[1] = joueur 2...
    std::vector<Piece>  pieces;
    std::mt19937        rng;      // générateur aléatoire propre au jeu
};

// ---------------------------------------------------------------------------
// TRICHES 😈 : elles ne profitent qu'au joueur 0 (l'hôte). Elles vivent dans
// la LOGIQUE, donc chez l'hôte seulement : c'est lui qui a l'autorité, et les
// clients ne font que recevoir le résultat. Un client ne peut pas tricher.
// ---------------------------------------------------------------------------
struct Triches {
    bool turbo        = false;  // l'hôte va 2 fois plus vite
    bool grandesMains = false;  // l'hôte ramasse de plus loin (hitbox agrandie, sprite normal)
    bool aimant       = false;  // les pièces proches glissent vers l'hôte
    bool gel          = false;  // les autres joueurs ne peuvent plus bouger
    bool inversion    = false;  // les commandes des autres sont inversées
};

// Nouvelle partie avec nbJoueurs joueurs (entre 1 et MAX_JOUEURS)
void initialiser(Jeu& jeu, unsigned graine, int nbJoueurs);

// Avance le jeu d'un pas. entrees[i] = ce que veut faire le joueur i
// (entrees doit contenir exactement un élément par joueur).
// "triches" a une valeur par défaut : sans triche, on appelle comme avant.
void mettreAJour(Jeu& jeu, const std::vector<Entrees>& entrees, float dt,
                 const Triches& triches = Triches{});

// Triche instantanée "pluie de pièces" : toutes les pièces viennent sur le
// joueur 0, qui les ramassera toutes au pas suivant
void pluieDePieces(Jeu& jeu);

// Somme des scores de tous les joueurs (pratique pour détecter un ramassage)
int scoreTotal(const Jeu& jeu);
