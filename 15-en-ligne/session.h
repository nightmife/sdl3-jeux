// La session réseau : être HÔTE (ouvrir une salle, accepter des joueurs)
// ou CLIENT (rejoindre la salle de quelqu'un). Tout est NON BLOQUANT :
// ces fonctions sont appelées à chaque frame et rendent la main tout de suite.
#pragma once

#include <string>
#include <vector>

#include "jeu.h"  // Entrees
#include "reseau.h"

// ---------------------------------------------------------------------------
// Côté hôte
// ---------------------------------------------------------------------------

// Un joueur connecté à notre salle, vu par l'hôte
struct JoueurDistant {
    NET_StreamSocket* socket = nullptr;
    Reception         reception;  // un tampon de réception par connexion
    Entrees           entrees;    // dernières touches reçues de ce joueur
};

struct Hote {
    NET_Server*                serveur = nullptr;
    std::vector<JoueurDistant> joueurs;  // joueurs[k] sera le joueur k+2 (l'hôte est le joueur 1)
};

// Ouvre la salle sur le port PORT. False en cas d'échec (port déjà pris...).
bool demarrerHote(Hote& hote);

// Accepte TOUS les joueurs qui attendent à la porte, sans jamais attendre.
// À appeler à chaque frame tant que la salle est ouverte.
void accepterNouveauxJoueurs(Hote& hote);

// Raccroche avec tout le monde et ferme la salle
void fermerHote(Hote& hote);

// ---------------------------------------------------------------------------
// Côté client
// ---------------------------------------------------------------------------

enum class EtatClient {
    Inactif,
    Resolution,  // on cherche l'adresse (DNS)
    Connexion,   // on attend que l'hôte décroche
    Connecte,
    Echec,       // voir "erreur"
};

struct Client {
    EtatClient        etat    = EtatClient::Inactif;
    NET_Address*      adresse = nullptr;
    NET_StreamSocket* socket  = nullptr;
    Reception         reception;
    std::string       erreur;
};

// Lance la connexion vers "nomHote" (sans attendre le résultat)
void demarrerClient(Client& client, const char* nomHote);

// Fait avancer la connexion (résolution puis connexion) sans bloquer.
// À appeler à chaque frame : client.etat dit où on en est.
void avancerClient(Client& client);

// Raccroche et libère tout
void fermerClient(Client& client);
