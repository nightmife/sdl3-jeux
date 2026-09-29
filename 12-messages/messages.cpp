#include "messages.h"

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
