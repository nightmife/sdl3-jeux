#include "messages.h"

#include <cstring>

// ---------------------------------------------------------------------------
// Ecrivain
// ---------------------------------------------------------------------------

void Ecrivain::u8(std::uint8_t v)
{
    if (!ok || pos + 1 > capacite) { ok = false; return; }
    tampon[pos++] = v;
}

void Ecrivain::u16(std::uint16_t v)
{
    u8(static_cast<std::uint8_t>(v >> 8));    // poids fort d'abord (ordre réseau)
    u8(static_cast<std::uint8_t>(v & 0xFF));
}

void Ecrivain::u32(std::uint32_t v)
{
    // TODO(human)
}

void Ecrivain::f32(float v)
{
}

// ---------------------------------------------------------------------------
// Lecteur
// ---------------------------------------------------------------------------

std::uint8_t Lecteur::u8()
{
    if (!ok || pos + 1 > taille) { ok = false; return 0; }
    return tampon[pos++];
}

std::uint16_t Lecteur::u16()
{
    const std::uint16_t fort   = u8();
    const std::uint16_t faible = u8();
    return static_cast<std::uint16_t>((fort << 8) | faible);
}

std::uint32_t Lecteur::u32()
{
}

float Lecteur::f32()
{
}

// ---------------------------------------------------------------------------
// Message d'entrées
// ---------------------------------------------------------------------------

// Numéro du bit de chaque touche dans l'octet d'entrées
constexpr std::uint8_t BIT_HAUT   = 0;
constexpr std::uint8_t BIT_BAS    = 1;
constexpr std::uint8_t BIT_GAUCHE = 2;
constexpr std::uint8_t BIT_DROITE = 3;

int encoderEntrees(const Entrees& e, std::uint8_t* tampon)
{
    tampon[0] = static_cast<std::uint8_t>(TypeMessage::Entrees);

    std::uint8_t bits = 0;
    if (e.haut) bits |= (1 << BIT_HAUT);
    if (e.bas) bits |= (1 << BIT_BAS);
    if (e.gauche) bits |= (1 << BIT_GAUCHE);
    if (e.droite) bits |= (1 << BIT_DROITE);

    tampon[1] = bits;

    return TAILLE_MESSAGE_ENTREES;
}

bool decoderEntrees(const std::uint8_t* tampon, int taille, Entrees& e)
{
    if (taille != TAILLE_MESSAGE_ENTREES) return false;
    if (tampon[0] != static_cast<std::uint8_t>(TypeMessage::Entrees)) return false;

    e.haut = (tampon[1] >> BIT_HAUT) & 1;
    e.bas = (tampon[1] >> BIT_BAS) & 1;
    e.gauche = (tampon[1] >> BIT_GAUCHE) & 1;
    e.droite = (tampon[1] >> BIT_DROITE) & 1;

    return true;
}

// ---------------------------------------------------------------------------
// Message d'état (étape suivante)
// ---------------------------------------------------------------------------

int encoderEtat(const Jeu& jeu, std::uint8_t* tampon, int capacite)
{
    (void)jeu; (void)tampon; (void)capacite;
    return -1;
}

bool decoderEtat(const std::uint8_t* tampon, int taille, Jeu& jeu)
{
    (void)tampon; (void)taille; (void)jeu;
    return false;
}
