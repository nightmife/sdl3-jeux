// Encodage des données du jeu en octets (sérialisation), et l'inverse.
// Aucune dépendance au réseau ni à SDL : testable seul, comme jeu.cpp.
#pragma once

#include <cstdint>
#include "jeu.h"

// Le 1er octet de chaque message dit ce qu'il contient
enum class TypeMessage : std::uint8_t {
    Entrees = 1,  // client -> hôte : les touches enfoncées
    Etat    = 2,  // hôte -> clients : positions, pièces, scores
};

// ---------------------------------------------------------------------------
// Outils d'écriture et de lecture, octet par octet, en ordre réseau
// ---------------------------------------------------------------------------

// Écrit des valeurs les unes à la suite des autres dans un tampon.
// Si le tampon est trop petit, "ok" passe à false et plus rien n'est écrit.
struct Ecrivain {
    std::uint8_t* tampon;
    int           capacite;
    int           pos = 0;     // prochain octet à écrire = nombre d'octets écrits
    bool          ok  = true;

    void u8(std::uint8_t v);
    void u16(std::uint16_t v);
    void u32(std::uint32_t v);
    void f32(float v);
};

// Relit des valeurs dans l'ordre où elles ont été écrites.
// Si on essaie de lire au-delà de la fin, "ok" passe à false et on lit 0.
struct Lecteur {
    const std::uint8_t* tampon;
    int                 taille;
    int                 pos = 0;
    bool                ok  = true;

    std::uint8_t  u8();
    std::uint16_t u16();
    std::uint32_t u32();
    float         f32();
};

// ---------------------------------------------------------------------------
// Messages
// ---------------------------------------------------------------------------

// Message d'entrées : [type][4 bits : haut, bas, gauche, droite]
constexpr int TAILLE_MESSAGE_ENTREES = 2;
int  encoderEntrees(const Entrees& e, std::uint8_t* tampon);
bool decoderEntrees(const std::uint8_t* tampon, int taille, Entrees& e);

// Message d'état : [type][nbJoueurs][x y score]...[nbPieces][x y]...
// Renvoie le nombre d'octets écrits, ou -1 si le tampon est trop petit.
int  encoderEtat(const Jeu& jeu, std::uint8_t* tampon, int capacite);
// Remplit joueurs et pieces de "jeu" (le reste de "jeu" n'est pas touché).
bool decoderEtat(const std::uint8_t* tampon, int taille, Jeu& jeu);
