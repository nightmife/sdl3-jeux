// Encodage des données du jeu en octets (sérialisation), et l'inverse.
// Aucune dépendance au réseau ni à SDL : testable seul, comme jeu.cpp.
#pragma once

#include <cstdint>
#include "jeu.h"

// Le 1er octet de chaque message dit ce qu'il contient
enum class TypeMessage : std::uint8_t {
    Entrees = 1,  // client -> hôte : les touches enfoncées
};

// Message d'entrées : [type][4 bits : haut, bas, gauche, droite]
constexpr int TAILLE_MESSAGE_ENTREES = 2;

// Écrit le message dans tampon (au moins TAILLE_MESSAGE_ENTREES octets).
// Renvoie le nombre d'octets écrits.
int encoderEntrees(const Entrees& e, std::uint8_t* tampon);

// Relit un message d'entrées. False si ce n'est pas un message d'entrées valide.
bool decoderEntrees(const std::uint8_t* tampon, int taille, Entrees& e);
