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

// Ajoute NB_PIECES pièces à des positions aléatoires, jamais sur la zone interdite
static void genererPieces(Jeu& jeu, const Rect& zoneInterdite)
{
    for (int i = 0; i < NB_PIECES; ++i) {
        Piece p;
        // Nombre aléatoire uniforme dans [0, max[, tiré avec le générateur du jeu
        // (jeu.rng) : même graine => mêmes pièces, ce qui compte pour le réseau.
        std::uniform_real_distribution<float> aleaX(0.0f, LARGEUR_MONDE - p.taille);
        std::uniform_real_distribution<float> aleaY(0.0f, HAUTEUR_MONDE - p.taille);
        // Échantillonnage par REJET : tirer, et recommencer tant que ça tombe
        // sur la zone interdite. do...while : on tire AU MOINS une fois.
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

// Avance le jeu de dt secondes : déplacement, bords, ramassage, vagues
void mettreAJour(Jeu& jeu, const Entrees& entrees, float dt)
{
    Joueur& joueur = jeu.joueur;  // référence : on modifie le VRAI joueur, pas une copie

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

    // Ramassage par SWAP-AND-POP : pour retirer la pièce i sans tout décaler
    // (erase coûte O(n)), on l'écrase avec la dernière puis on retire la
    // dernière (O(1)). L'ordre des pièces change, mais on s'en moque.
    // - On ne fait PAS i++ après une suppression : la pièce venue à l'indice i
    //   n'a pas encore été testée.
    // - La condition i < size() est réévaluée à chaque tour car la taille diminue.
    // - Jamais de suppression dans un foreach : ses itérateurs seraient invalidés.
    Rect rJoueur = joueur.hitbox();
    size_t i = 0;  // size_t, comme size() : pas de comparaison signé/non signé
    while (i < jeu.pieces.size()) {
        if (collision(rJoueur, jeu.pieces[i].hitbox())) {
            jeu.pieces[i] = jeu.pieces.back();  // écraser la pièce i avec la dernière
            jeu.pieces.pop_back();              // retirer la dernière (en double)
            jeu.score += 1;
        }
        else { i++; }
    }

    // Nouvelle vague quand toutes les pièces sont ramassées (APRÈS le ramassage,
    // pour réagir dès la frame où la dernière pièce disparaît)
    if (jeu.pieces.empty()) { genererPieces(jeu, rJoueur); }
}
