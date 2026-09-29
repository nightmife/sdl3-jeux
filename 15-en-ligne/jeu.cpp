#include "jeu.h"

#include <cmath>

// Vrai si les deux rectangles se chevauchent (collision AABB).
// Deux rectangles se touchent s'ils se chevauchent sur l'axe X ET sur l'axe Y :
//   - sur X : le bord gauche de A est avant le bord droit de B,
//             ET le bord droit de A est après le bord gauche de B ;
//   - pareil sur Y.
// C'est un ET (&&) : se chevaucher sur un seul axe (même ligne ou même
// colonne) ne suffit pas. Avec <= / >=, se toucher pile par un bord compte.
static bool collision(const Rect& a, const Rect& b)
{
    return (a.x <= b.x + b.w && a.x + a.w >= b.x    // chevauchement horizontal
         && a.y <= b.y + b.h && a.y + a.h >= b.y);  // chevauchement vertical
}

// Vrai si le rectangle touche au moins un joueur
static bool toucheUnJoueur(const Jeu& jeu, const Rect& r)
{
    for (const Joueur& j : jeu.joueurs)
        if (j.actif && collision(j.hitbox(), r)) return true;
    return false;
}

// Ajoute NB_PIECES pièces à des positions aléatoires, jamais sur un joueur
static void genererPieces(Jeu& jeu)
{
    for (int i = 0; i < NB_PIECES; ++i) {
        Piece p;
        // Tirage avec le générateur du jeu : même graine => mêmes pièces
        std::uniform_real_distribution<float> aleaX(0.0f, LARGEUR_MONDE - p.taille);
        std::uniform_real_distribution<float> aleaY(0.0f, HAUTEUR_MONDE - p.taille);
        // Échantillonnage par REJET : on retire tant qu'on tombe sur un joueur
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
    // 1. Direction voulue. Des if SÉPARÉS (pas de else) : droite + gauche
    //    s'annulent, haut + droite donnent une diagonale.
    float dx = 0.f, dy = 0.f;
    if (entrees.droite) { dx += 1; }
    if (entrees.gauche) { dx -= 1; }
    if (entrees.haut)   { dy -= 1; }  // y va vers le BAS à l'écran : monter = y diminue
    if (entrees.bas)    { dy += 1; }

    // 2. Normalisation : en diagonale, (1, 1) a une longueur de √2 ≈ 1,41, on
    //    irait 41 % plus vite. On divise par la longueur pour la ramener à 1.
    //    On teste la NORME (dx + dy vaut 0 pour la diagonale (1, -1) !) :
    //    diviser par 0 donnerait NaN, qui contamine tous les calculs suivants.
    float norme = std::sqrt((dx * dx) + (dy * dy));
    if (norme > 0) {
        dx /= norme;
        dy /= norme;
    }

    // 3. Mouvement : VITESSE est en pixels PAR SECONDE, dt en secondes.
    joueur.x += VITESSE * dx * dt;
    joueur.y += VITESSE * dy * dt;

    // 4. Garder le joueur dans le monde : on BOUGE D'ABORD, puis on CORRIGE
    //    (sinon, avec un grand dt, il pourrait dépasser d'un pas entier).
    //    x, y = coin haut-gauche : le max est la largeur MOINS la taille du joueur.
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

// Avance le jeu d'un pas. entrees[i] = ce que veut faire le joueur i.
void mettreAJour(Jeu& jeu, const std::vector<Entrees>& entrees, float dt)
{
    // 1. Chaque joueur se déplace selon SES entrées
    //    (les joueurs partis ne bougent plus)
    for (size_t i = 0; i < jeu.joueurs.size() && i < entrees.size(); ++i)
        if (jeu.joueurs[i].actif)
            deplacerJoueur(jeu.joueurs[i], entrees[i], dt);

    // 2. Ramassage : chaque pièce touchée disparaît et rapporte 1 point
    //    au joueur qui l'a touchée. Swap-and-pop comme en solo, avec une
    //    boucle intérieure pour chercher QUI touche la pièce i.
    size_t i = 0;
    while (i < jeu.pieces.size()) {
        bool ramassee = false;  // "messager" entre la boucle intérieure et l'extérieure
        for (Joueur &joueur : jeu.joueurs) {  // & : on modifie le score du VRAI joueur
            if (!joueur.actif) continue;       // un joueur parti ne ramasse plus rien
            if (collision(joueur.hitbox(), jeu.pieces[i].hitbox())) {
                joueur.score += 1;
                ramassee = true;
                // break : la pièce est prise, on arrête de chercher. Sans lui, deux
                // joueurs sur la même pièce marqueraient chacun un point.
                // Conséquence : à égalité, le premier joueur de la liste gagne.
                break;
            }
        }

        if (ramassee) {
            jeu.pieces[i] = jeu.pieces.back();  // swap-and-pop, sans i++ :
            jeu.pieces.pop_back();              // la nouvelle pièce i reste à tester
        }
        else i++;
    }

    // 3. Nouvelle vague quand toutes les pièces sont ramassées
    if (jeu.pieces.empty()) { genererPieces(jeu); }
}

int scoreTotal(const Jeu& jeu)
{
    int total = 0;
    for (const Joueur& j : jeu.joueurs) total += j.score;
    return total;
}
