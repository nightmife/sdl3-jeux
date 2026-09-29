#include "jeu.h"

#include <cmath>

// Vrai si les deux rectangles se chevauchent
static bool collision(const Rect& a, const Rect& b)
{
    return (a.x <= b.x + b.w && a.x + a.w >= b.x && a.y <= b.y + b.h && a.y + a.h >= b.y);
}

// Vrai si le rectangle touche au moins un joueur
static bool toucheUnJoueur(const Jeu& jeu, const Rect& r)
{
    for (const Joueur& j : jeu.joueurs)
        if (collision(j.hitbox(), r)) return true;
    return false;
}

// Ajoute NB_PIECES pièces à des positions aléatoires, jamais sur un joueur
static void genererPieces(Jeu& jeu)
{
    for (int i = 0; i < NB_PIECES; ++i) {
        Piece p;
        std::uniform_real_distribution<float> aleaX(0.0f, LARGEUR_MONDE - p.taille);
        std::uniform_real_distribution<float> aleaY(0.0f, HAUTEUR_MONDE - p.taille);
        do {
            p.x = aleaX(jeu.rng);
            p.y = aleaY(jeu.rng);
        } while (toucheUnJoueur(jeu, p.hitbox()));
        jeu.pieces.push_back(p);
    }
}

// Ton code de déplacement (leçon 2), appliqué à UN joueur
static void deplacerJoueur(Joueur& joueur, const Entrees& entrees, float dt)
{
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
}

void initialiser(Jeu& jeu, unsigned graine, int nbJoueurs)
{
    jeu = Jeu{};  // repart d'un état propre
    jeu.rng.seed(graine);

    if (nbJoueurs < 1) nbJoueurs = 1;
    if (nbJoueurs > MAX_JOUEURS) nbJoueurs = MAX_JOUEURS;

    // Joueurs répartis régulièrement sur une ligne horizontale, au milieu
    for (int i = 0; i < nbJoueurs; ++i) {
        Joueur j;
        j.x = LARGEUR_MONDE * static_cast<float>(i + 1) / static_cast<float>(nbJoueurs + 1) - j.taille / 2.0f;
        j.y = (HAUTEUR_MONDE - j.taille) / 2.0f;
        jeu.joueurs.push_back(j);
    }

    genererPieces(jeu);
}

void mettreAJour(Jeu& jeu, const std::vector<Entrees>& entrees, float dt)
{
    // 1. Chaque joueur se déplace selon SES entrées
    for (size_t i = 0; i < jeu.joueurs.size() && i < entrees.size(); ++i)
        deplacerJoueur(jeu.joueurs[i], entrees[i], dt);

    // 2. Ramassage : chaque pièce touchée disparaît et rapporte 1 point
    //    au joueur qui l'a touchée
    // TODO(human)

    // 3. Nouvelle vague quand toutes les pièces sont ramassées
    if (jeu.pieces.empty()) { genererPieces(jeu); }
}

int scoreTotal(const Jeu& jeu)
{
    int total = 0;
    for (const Joueur& j : jeu.joueurs) total += j.score;
    return total;
}
