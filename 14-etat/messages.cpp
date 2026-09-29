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
    // 4 octets, poids FORT d'abord (ordre réseau). >> k ramène l'octet voulu à
    // droite ; le static_cast vers uint8_t ne garde que ces 8 bits.
    // Pour le dernier : & 0xFF (GARDER les 8 bits de droite), pas >> !
    u8(static_cast<std::uint8_t>(v >> 24));
    u8(static_cast<std::uint8_t>(v >> 16));
    u8(static_cast<std::uint8_t>(v >> 8));
    u8(static_cast<std::uint8_t>(v & 0xFF));
}

void Ecrivain::f32(float v)
{
    // Un float fait 32 bits (format IEEE 754, le même partout). On COPIE ses
    // bits bruts dans un entier (memcpy(destination, source) = "bits = v"),
    // SANS conversion : (uint32_t)v transformerait 3.5 en 3.
    std::uint32_t bits;
    std::memcpy(&bits, &v, 4);
    u32(bits);
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
    // Relire les 4 octets DANS LE MÊME ORDRE qu'à l'écriture. Chaque u8()
    // avance "pos" (le marque-page) : c'est pour ça qu'appeler 4 fois la même
    // fonction lit 4 octets différents. Rangés en uint32_t pour que << 24 marche.
    const std::uint32_t o0 = u8(); 
    const std::uint32_t o1 = u8(); 
    const std::uint32_t o2 = u8(); 
    const std::uint32_t o3 = u8();

    // Remettre chaque octet à sa place (<<) puis les coller ensemble (|)
    return (o0 << 24) | (o1 << 16) | (o2 << 8) | o3;
}

float Lecteur::f32()
{
    std::uint32_t bits = u32();
    // Le miroir de Ecrivain::f32 : cette fois la destination est le float ("v = bits")
    float v;
    std::memcpy(&v, &bits, 4);
    return v;
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

    // Octet 1 : un bit par touche enfoncée (des « drapeaux »).
    // 1 << k = un nombre avec seulement le bit n°k à 1 ; |= l'allume dans "bits".
    // Exemple : haut + droite = bits 0 et 3 = 00001001.
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
    // Vérifier AVANT de lire : la bonne taille (sinon lire tampon[1] pourrait
    // sortir du tableau), puis le bon type de message.
    if (taille != TAILLE_MESSAGE_ENTREES) return false;
    if (tampon[0] != static_cast<std::uint8_t>(TypeMessage::Entrees)) return false;

    // Tester le bit n°k : >> k l'amène tout à droite, & 1 ne garde que lui (0 ou 1)
    e.haut = (tampon[1] >> BIT_HAUT) & 1;
    e.bas = (tampon[1] >> BIT_BAS) & 1;
    e.gauche = (tampon[1] >> BIT_GAUCHE) & 1;
    e.droite = (tampon[1] >> BIT_DROITE) & 1;

    return true;
}

// ---------------------------------------------------------------------------
// Message d'état : [type][nbJoueurs][x y score]...[nbPieces][x y]...
// ---------------------------------------------------------------------------

// Au-delà, un message d'état est forcément invalide (protection contre les abus)
constexpr int MAX_PIECES_MESSAGE = 64;

int encoderEtat(const Jeu& jeu, std::uint8_t* tampon, int capacite)
{
    Ecrivain e{tampon, capacite};
    e.u8(static_cast<std::uint8_t>(TypeMessage::Etat));

    // Format : [nbJoueurs][x y score]...[nbPieces][x y]...
    // Le décodeur devra lire EXACTEMENT dans cet ordre, avec ces types :
    // le message ne contient aucune étiquette, seulement des octets à la suite.
    e.u8(static_cast<std::uint8_t>(jeu.joueurs.size()));
    for (const Joueur &j : jeu.joueurs) {
        e.f32(j.x);
        e.f32(j.y);
        e.u16(static_cast<std::uint16_t>(j.score));
    }

    e.u8(static_cast<std::uint8_t>(jeu.pieces.size()));
    for (const Piece &p : jeu.pieces) {
        e.f32(p.x);
        e.f32(p.y);
    }

    // e.ok = tout a tenu dans le tampon ; e.pos = nombre d'octets écrits
    return e.ok ? e.pos : -1;
}

bool decoderEtat(const std::uint8_t* tampon, int taille, Jeu& jeu)
{
    Lecteur l{tampon, taille};
    if (l.u8() != static_cast<std::uint8_t>(TypeMessage::Etat)) return false;

    // Miroir de l'encodeur, ligne par ligne. Pour chaque nombre reçu :
    // LIRE -> VÉRIFIER (jamais confiance au réseau) -> resize -> remplir.
    const int nbJoueurs = l.u8();

    if (nbJoueurs > MAX_JOUEURS) return false;
    jeu.joueurs.resize(nbJoueurs);

    for (Joueur &j : jeu.joueurs) {
        j.x = l.f32();
        j.y = l.f32();
        j.score = l.u16();
    }

    // nbPieces est lu ICI, après tous les joueurs : c'est là que l'encodeur l'a écrit
    const int nbPieces = l.u8();

    if (nbPieces > MAX_PIECES_MESSAGE) return false;
    jeu.pieces.resize(nbPieces);

    for (Piece &p : jeu.pieces) {
        p.x = l.f32();
        p.y = l.f32();
    }
    
    // ok : on n'a pas lu au-delà de la fin ; pos == taille : ni trop, ni trop peu
    return l.ok && l.pos == l.taille;
}
