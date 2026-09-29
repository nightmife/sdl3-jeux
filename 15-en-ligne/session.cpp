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

std::vector<size_t> accepterNouveauxJoueurs(Hote& hote)
{
    std::vector<size_t> nouveaux;  // indices des joueurs arrivés pendant cet appel

    // Plusieurs joueurs peuvent arriver pendant la même frame : on décroche en
    // boucle jusqu'à ce qu'il n'y ait plus personne. Aucune attente ici
    // (surtout pas NET_WaitUntilInputAvailable, qui gèlerait la fenêtre).
    for (;;) {
        NET_StreamSocket *client = nullptr;

        if (!NET_AcceptClient(hote.serveur, &client)) {
            SDL_Log("Erreur en acceptant un joueur: %s", SDL_GetError());
            return nouveaux;
        }
        
        if (client == nullptr) return nouveaux;  // plus personne à la porte pour cette frame

        // Trouver une place pour le nouveau venu, dans cet ordre :
        //   1. une place LIBRE (socket == nullptr, laissée par un joueur parti) :
        //      on la réutilise, ce qui garde les numéros des autres inchangés ;
        //   2. sinon une nouvelle place, si la salle n'est pas pleine
        //      (l'hôte est le joueur 1 : il reste MAX_JOUEURS - 1 places) ;
        //   3. sinon refus : on raccroche, et "continue" pour traiter ceux
        //      qui attendent peut-être derrière.
        bool trouvee = false;
        size_t place;
        // place < size() (et surtout pas size() - 1 : sur un vector vide,
        // 0 - 1 en size_t donne un nombre gigantesque)
        for (place = 0; place < hote.joueurs.size(); place ++) {
            if (hote.joueurs[place].socket == nullptr) {
                trouvee = true;
                break;
            }
        }
        
        if (trouvee) {
            hote.joueurs[place] = JoueurDistant{};  // vider la reception et les
            hote.joueurs[place].socket = client;    // entrées de l'ancien occupant
        } else if (hote.joueurs.size() < MAX_JOUEURS - 1) {
            JoueurDistant j;
            j.socket = client;
            hote.joueurs.push_back(j);
            place = hote.joueurs.size() - 1;
        } else {
            SDL_Log("Salle pleine: joueur refusé");
            NET_DestroyStreamSocket(client);
            continue;
        }

        nouveaux.push_back(place);
        SDL_Log("Joueur %d connecté !", static_cast<int>(place) + 2);
    }
}

int nbConnectes(const Hote& hote)
{
    int n = 0;
    for (const JoueurDistant& j : hote.joueurs)
        if (j.socket) ++n;
    return n;
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
