#include "session.h"

#include "jeu.h"  // MAX_JOUEURS

// ---------------------------------------------------------------------------
// Côté hôte
// ---------------------------------------------------------------------------

bool demarrerHote(Hote& hote)
{
    hote.serveur = NET_CreateServer(nullptr, PORT, 0);
    if (!hote.serveur) {
        SDL_Log("Impossible d'ouvrir la salle : %s", SDL_GetError());
        return false;
    }
    SDL_Log("Salle ouverte sur le port %d", PORT);
    return true;
}

void accepterNouveauxJoueurs(Hote& hote)
{

    // Plusieurs joueurs peuvent arriver pendant la même frame : on décroche en
    // boucle jusqu'à ce qu'il n'y ait plus personne. Aucune attente ici
    // (surtout pas NET_WaitUntilInputAvailable, qui gèlerait la fenêtre).
    for (;;) {
        NET_StreamSocket *client = nullptr;

        if (!NET_AcceptClient(hote.serveur, &client)) {
            SDL_Log("Erreur en acceptant un joueur: %s", SDL_GetError());
            return;
        }
        
        if (client == nullptr) return;  // plus personne à la porte pour cette frame

        // Salle pleine : l'hôte est le joueur 1, il reste MAX_JOUEURS - 1 places
        // (attention à l'erreur de un !). On raccroche au nez du joueur en trop,
        // puis "continue" pour traiter ceux qui attendent peut-être derrière.
        if (hote.joueurs.size() >= MAX_JOUEURS - 1) {
            SDL_Log("Salle pleine: joueur refusé");
            NET_DestroyStreamSocket(client);
            continue;
        }

        JoueurDistant j;
        j.socket = client;
        hote.joueurs.push_back(j);
        SDL_Log("Joueur %d connecté !", static_cast<int>(hote.joueurs.size()) + 1);
    }
}

void fermerHote(Hote& hote)
{
    for (JoueurDistant& j : hote.joueurs)
        if (j.socket) NET_DestroyStreamSocket(j.socket);  // nullptr = déjà parti
    hote.joueurs.clear();
    if (hote.serveur) NET_DestroyServer(hote.serveur);
    hote.serveur = nullptr;
}

// ---------------------------------------------------------------------------
// Côté client
// ---------------------------------------------------------------------------

void demarrerClient(Client& client, const char* nomHote)
{
    fermerClient(client);  // repartir de zéro si une tentative précédente existait
    client.adresse = NET_ResolveHostname(nomHote);
    if (!client.adresse) {
        client.etat   = EtatClient::Echec;
        client.erreur = SDL_GetError();
        return;
    }
    client.etat = EtatClient::Resolution;
}

void avancerClient(Client& client)
{
    switch (client.etat) {
    case EtatClient::Resolution: {
        // Même idée que NET_WaitUntilResolved, mais on REGARDE au lieu d'attendre
        const NET_Status s = NET_GetAddressStatus(client.adresse);
        if (s == NET_FAILURE) {
            client.etat   = EtatClient::Echec;
            client.erreur = std::string("adresse introuvable : ") + SDL_GetError();
        } else if (s == NET_SUCCESS) {
            client.socket = NET_CreateClient(client.adresse, PORT, 0);
            if (!client.socket) {
                client.etat   = EtatClient::Echec;
                client.erreur = SDL_GetError();
            } else {
                client.etat = EtatClient::Connexion;
            }
        }
        break;  // NET_WAITING : rien à faire, on regardera à la prochaine frame
    }
    case EtatClient::Connexion: {
        const NET_Status s = NET_GetConnectionStatus(client.socket);
        if (s == NET_FAILURE) {
            client.etat   = EtatClient::Echec;
            client.erreur = std::string("connexion refusée : ") + SDL_GetError();
        } else if (s == NET_SUCCESS) {
            client.etat = EtatClient::Connecte;
            SDL_Log("Connecté à l'hôte !");
        }
        break;
    }
    default:
        break;
    }
}

void fermerClient(Client& client)
{
    if (client.socket)  NET_DestroyStreamSocket(client.socket);
    if (client.adresse) NET_UnrefAddress(client.adresse);
    client.socket    = nullptr;
    client.adresse   = nullptr;
    client.reception = Reception{};
    client.etat      = EtatClient::Inactif;
    client.erreur.clear();
}
