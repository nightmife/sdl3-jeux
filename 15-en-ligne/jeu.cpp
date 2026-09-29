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
static void deplacerJoueur(Joueur& joueur, const Entrees& entrees, float dt, float vitesse)
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

    // 3. Mouvement : la vitesse est en pixels PAR SECONDE, dt en secondes.
    //    (d'habitude VITESSE, ou plus si la triche turbo est active)
    joueur.x += vitesse * dx * dt;
    joueur.y += vitesse * dy * dt;

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

// Triche "inversion" : haut <-> bas, gauche <-> droite
static Entrees inverser(const Entrees& e)
{
    Entrees r;
    r.haut   = e.bas;
    r.bas    = e.haut;
    r.gauche = e.droite;
    r.droite = e.gauche;
    return r;
}

// Triche "aimant" : les pièces à moins de RAYON pixels glissent vers le joueur
static void attirerPieces(Jeu& jeu, const Joueur& cible, float dt)
{
    constexpr float RAYON = 300.0f;             // portée de l'aimant
    constexpr float VITESSE_AIMANT = 350.0f;    // vitesse des pièces attirées, en px/s
    const float cx = cible.x + cible.taille / 2.0f;  // centre du joueur
    const float cy = cible.y + cible.taille / 2.0f;
    for (Piece& p : jeu.pieces) {
        const float dx = cx - (p.x + p.taille / 2.0f);  // vecteur pièce -> joueur
        const float dy = cy - (p.y + p.taille / 2.0f);
        const float distance = std::sqrt(dx * dx + dy * dy);
        if (distance > 0 && distance < RAYON) {
            // On normalise (comme pour le joueur) pour avancer à vitesse constante
            p.x += dx / distance * VITESSE_AIMANT * dt;
            p.y += dy / distance * VITESSE_AIMANT * dt;
        }
    }
}

// Avance le jeu d'un pas. entrees[i] = ce que veut faire le joueur i.
void mettreAJour(Jeu& jeu, const std::vector<Entrees>& entrees, float dt, const Triches& triches)
{
    // 1. Chaque joueur se déplace selon SES entrées
    //    (les joueurs partis ne bougent plus)
    for (size_t i = 0; i < jeu.joueurs.size() && i < entrees.size(); ++i) {
        if (!jeu.joueurs[i].actif) continue;
        const bool estHote = (i == 0);

        if (!estHote && triches.gel) continue;  // triche gel : les autres sont figés

        // Triche inversion : on modifie les entrées AVANT de les appliquer
        const Entrees e = (!estHote && triches.inversion) ? inverser(entrees[i]) : entrees[i];
        // Triche turbo : l'hôte va 2 fois plus vite
        const float vitesse = (estHote && triches.turbo) ? 2.0f * VITESSE : VITESSE;
        deplacerJoueur(jeu.joueurs[i], e, dt, vitesse);
    }

    // Triche aimant : les pièces proches de l'hôte glissent vers lui
    if (triches.aimant && !jeu.joueurs.empty() && jeu.joueurs[0].actif)
        attirerPieces(jeu, jeu.joueurs[0], dt);

    // 2. Ramassage : chaque pièce touchée disparaît et rapporte 1 point
    //    au joueur qui l'a touchée. Swap-and-pop comme en solo, avec une
    //    boucle intérieure pour chercher QUI touche la pièce i.
    size_t i = 0;
    while (i < jeu.pieces.size()) {
        bool ramassee = false;  // "messager" entre la boucle intérieure et l'extérieure
        for (Joueur &joueur : jeu.joueurs) {  // & : on modifie le score du VRAI joueur
            if (!joueur.actif) continue;       // un joueur parti ne ramasse plus rien
            // Triche grandes mains : la hitbox de l'hôte (joueurs[0]) est agrandie de
            // MARGE pixels de chaque côté. On compare les ADRESSES pour savoir si
            // "joueur" est l'hôte (le foreach ne donne pas l'indice).
            Rect zone = joueur.hitbox();
            if (triches.grandesMains && &joueur == &jeu.joueurs[0]) {
                constexpr float MARGE = 60.0f;
                zone = Rect{zone.x - MARGE, zone.y - MARGE, zone.w + 2 * MARGE, zone.h + 2 * MARGE};
            }
            if (collision(zone, jeu.pieces[i].hitbox())) {
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

#ifdef AVEC_TRICHES
void pluieDePieces(Jeu& jeu)
{
    if (jeu.joueurs.empty() || !jeu.joueurs[0].actif) return;
    const Joueur& hote = jeu.joueurs[0];
    // Chaque pièce est posée au centre de l'hôte : au prochain pas, le
    // ramassage les lui donnera toutes (c'est la logique normale qui compte les points)
    for (Piece& p : jeu.pieces) {
        p.x = hote.x + (hote.taille - p.taille) / 2.0f;
        p.y = hote.y + (hote.taille - p.taille) / 2.0f;
    }
}
#endif  // AVEC_TRICHES
