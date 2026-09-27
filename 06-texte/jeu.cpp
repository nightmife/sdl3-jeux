#include "jeu.h"

#include <cmath>

// Vrai si les deux rectangles se chevauchent
static bool collision(const Rect& a, const Rect& b)
{
    return (a.x <= b.x + b.w && a.x + a.w >= b.x && a.y <= b.y + b.h && a.y + a.h >= b.y);
}

// Ajoute NB_PIECES pièces à des positions aléatoires, jamais sur la zone interdite
static void genererPieces(Jeu& jeu, const Rect& zoneInterdite)
{
    for (int i = 0; i < NB_PIECES; ++i) {
        Piece p;
        // Nombre aléatoire uniforme dans [0, max[, tiré avec le générateur du jeu
        std::uniform_real_distribution<float> aleaX(0.0f, LARGEUR_MONDE - p.taille);
        std::uniform_real_distribution<float> aleaY(0.0f, HAUTEUR_MONDE - p.taille);
        do {
            p.x = aleaX(jeu.rng);
            p.y = aleaY(jeu.rng);
        } while (collision(zoneInterdite, p.hitbox()));
        jeu.pieces.push_back(p);
    }
}

void initialiser(Jeu& jeu, unsigned graine)
{
    jeu = Jeu{};  // repart d'un état propre
    jeu.rng.seed(graine);

    // Joueur au centre du monde
    jeu.joueur.x = (LARGEUR_MONDE - jeu.joueur.taille) / 2.0f;
    jeu.joueur.y = (HAUTEUR_MONDE - jeu.joueur.taille) / 2.0f;

    genererPieces(jeu, jeu.joueur.hitbox());
}

void mettreAJour(Jeu& jeu, const Entrees& entrees, float dt)
{
    Joueur& joueur = jeu.joueur;

    // Déplacement : direction (dx, dy) normalisée, puis vitesse * dt
    float dx = 0.f, dy = 0.f;
    if (entrees.droite) { dx += 1; }
    if (entrees.gauche) { dx -= 1; }
    if (entrees.haut)   { dy -= 1; }
    if (entrees.bas)    { dy += 1; }

    float norme = std::sqrt((dx * dx) + (dy * dy));
    if (norme > 0) {
        dx /= norme;
        dy /= norme;
    }

    joueur.x += VITESSE * dx * dt;
    joueur.y += VITESSE * dy * dt;

    // Garder le joueur dans le monde
    if (joueur.x > LARGEUR_MONDE - joueur.taille) { joueur.x = LARGEUR_MONDE - joueur.taille; }
    if (joueur.y > HAUTEUR_MONDE - joueur.taille) { joueur.y = HAUTEUR_MONDE - joueur.taille; }
    if (joueur.x < 0.f) { joueur.x = 0.f; }
    if (joueur.y < 0.f) { joueur.y = 0.f; }

    // Ramassage : swap-and-pop des pièces touchées
    Rect rJoueur = joueur.hitbox();
    size_t i = 0;
    while (i < jeu.pieces.size()) {
        if (collision(rJoueur, jeu.pieces[i].hitbox())) {
            jeu.pieces[i] = jeu.pieces.back();
            jeu.pieces.pop_back();
            jeu.score += 1;
        }
        else { i++; }
    }

    // Nouvelle vague quand toutes les pièces sont ramassées
    if (jeu.pieces.empty()) { genererPieces(jeu, rJoueur); }
}
