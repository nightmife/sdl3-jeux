#include "messages.h"

// Numéro du bit de chaque touche dans l'octet d'entrées
constexpr std::uint8_t BIT_HAUT   = 0;
constexpr std::uint8_t BIT_BAS    = 1;
constexpr std::uint8_t BIT_GAUCHE = 2;
constexpr std::uint8_t BIT_DROITE = 3;

int encoderEntrees(const Entrees& e, std::uint8_t* tampon)
{
    // TODO(human)
}

bool decoderEntrees(const std::uint8_t* tampon, int taille, Entrees& e)
{
}
