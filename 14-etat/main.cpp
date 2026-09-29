// Leçon 6, étape B : tests de la sérialisation, sans réseau ni fenêtre.
// On encode des valeurs, on les décode, et on vérifie qu'on retrouve les mêmes.

#include <cstdio>
#include <cstring>

#include "messages.h"

static int nbErreurs = 0;

static void verifier(bool condition, const char* description)
{
    std::printf("  %s %s\n", condition ? "OK  " : "RATÉ", description);
    if (!condition) ++nbErreurs;
}

static void testerEntiersEtFloats()
{
    std::printf("Entiers et floats :\n");
    std::uint8_t tampon[64];
    Ecrivain e{tampon, sizeof(tampon)};
    e.u32(0x12345678);
    e.f32(3.5f);
    e.f32(-1234.5678f);
    e.u32(4000000000u);
    verifier(e.ok && e.pos == 16, "16 octets écrits");
    verifier(tampon[0] == 0x12 && tampon[1] == 0x34 && tampon[2] == 0x56 && tampon[3] == 0x78,
             "u32 en ordre réseau (0x12 0x34 0x56 0x78)");
    verifier(tampon[4] == 0x40 && tampon[5] == 0x60 && tampon[6] == 0x00 && tampon[7] == 0x00,
             "3.5f s'écrit 0x40 0x60 0x00 0x00");

    Lecteur l{tampon, e.pos};
    verifier(l.u32() == 0x12345678, "relire 0x12345678");
    verifier(l.f32() == 3.5f, "relire 3.5f");
    verifier(l.f32() == -1234.5678f, "relire -1234.5678f (exactement le même float)");
    verifier(l.u32() == 4000000000u, "relire 4000000000");
    verifier(l.ok && l.pos == l.taille, "tout relu, sans dépasser");
    l.u8();
    verifier(!l.ok, "lire au-delà de la fin est détecté");

    std::uint8_t petit[3];
    Ecrivain p{petit, sizeof(petit)};
    p.u32(1);
    verifier(!p.ok, "écrire un u32 dans 3 octets est refusé");
}

static void testerEtat()
{
    std::printf("État du jeu :\n");
    Jeu original;
    initialiser(original, 42, 2);
    original.joueurs[0].score = 7;
    original.joueurs[1].score = 300;
    original.joueurs[1].x = 123.456f;

    std::uint8_t tampon[1024];
    const int taille = encoderEtat(original, tampon, sizeof(tampon));
    std::printf("  (message d'état : %d octets)\n", taille);
    verifier(taille > 0, "encoderEtat réussit");

    Jeu copie;
    verifier(decoderEtat(tampon, taille, copie), "decoderEtat réussit");
    bool identiques = copie.joueurs.size() == original.joueurs.size()
                   && copie.pieces.size() == original.pieces.size();
    for (size_t i = 0; identiques && i < original.joueurs.size(); ++i)
        identiques = copie.joueurs[i].x == original.joueurs[i].x
                  && copie.joueurs[i].y == original.joueurs[i].y
                  && copie.joueurs[i].score == original.joueurs[i].score;
    for (size_t i = 0; identiques && i < original.pieces.size(); ++i)
        identiques = copie.pieces[i].x == original.pieces[i].x
                  && copie.pieces[i].y == original.pieces[i].y;
    verifier(identiques, "joueurs et pièces identiques après l'aller-retour");

    verifier(taille <= 0 || !decoderEtat(tampon, taille - 1, copie), "message tronqué refusé");
    tampon[0] = static_cast<std::uint8_t>(TypeMessage::Entrees);
    verifier(!decoderEtat(tampon, taille, copie), "mauvais type refusé");
}

int main()
{
    testerEntiersEtFloats();
    testerEtat();
    std::printf("\n%s (%d erreur%s)\n", nbErreurs == 0 ? "TOUT EST BON" : "IL RESTE DES ERREURS",
                nbErreurs, nbErreurs > 1 ? "s" : "");
    return nbErreurs == 0 ? 0 : 1;
}
