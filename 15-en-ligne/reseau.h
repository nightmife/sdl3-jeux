// Ton protocole de la leçon 6 : chaque message = [taille sur 2 octets][données].
// Déplacé ici pour être réutilisé par tous les programmes réseau.
#pragma once

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>

// Le port de notre jeu
constexpr Uint16 PORT = 7777;

// Taille maximale des données d'un message (sans l'en-tête de 2 octets)
constexpr int TAILLE_MAX_MESSAGE = 1024;

// Octets reçus mais pas encore découpés en messages complets (un par connexion)
struct Reception {
    Uint8 octets[4 * (2 + TAILLE_MAX_MESSAGE)];
    int   nb = 0;  // nombre d'octets valides au début de "octets"
};

// Envoie [taille][données]. False si la connexion est cassée ou si c'est trop long.
bool envoyerMessage(NET_StreamSocket* socket, const void* donnees, int taille);

// Sort un message complet de "reception" : sa taille, -1 si incomplet, -2 si trop gros.
int extraireMessage(Reception& reception, void* dest, int tailleMax);

// NON BLOQUANT : ajoute à "reception" les octets arrivés depuis la dernière fois
// (éventuellement aucun), sans jamais attendre. False si la connexion est cassée.
// Ensuite, on appelle extraireMessage en boucle pour sortir les messages complets.
bool lireDisponible(NET_StreamSocket* socket, Reception& reception);
