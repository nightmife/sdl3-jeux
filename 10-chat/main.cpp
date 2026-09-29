// Leçon 6, étape 1 : premier contact réseau avec SDL3_net (TCP).
//
// Un seul programme, deux rôles :
//   ./chat hote              -> ouvre une "salle" et attend qu'un client s'y connecte
//   ./chat client <adresse>  -> se connecte à l'hôte (127.0.0.1 = ta propre machine)
//
// Programme en terminal, sans fenêtre : on utilise un main() classique et des
// fonctions qui ATTENDENT (NET_WaitUntil...), c'est plus simple pour apprendre.
// Dans le jeu, on ne pourra pas attendre comme ça (la fenêtre gèlerait).

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_net/SDL_net.h>

// Le "numéro de porte" de notre application sur la machine de l'hôte.
// Hôte et client doivent utiliser le même.
constexpr Uint16 PORT = 7777;

// ---------------------------------------------------------------------------
// Côté HÔTE : ouvrir la salle, attendre un client, l'accepter
// ---------------------------------------------------------------------------
static bool lancerHote()
{
    NET_Server *serveur = NET_CreateServer(nullptr, PORT, 0);
    if (serveur == nullptr) {
        SDL_Log("Impossible d'ouvrir la salle: %s", SDL_GetError());

        return false;
    }
    SDL_Log("Salle ouverte sur le port %d", PORT);

    void *aSurveiller[] = { serveur };
    NET_WaitUntilInputAvailable(aSurveiller, 1, -1);

    NET_StreamSocket *client = nullptr;
    if (!NET_AcceptClient(serveur, &client) || client == nullptr) {
        SDL_Log("Aucun joueur");
        
        NET_DestroyServer(serveur);

        return false;
    }

    NET_Address *adr = NET_GetStreamSocketAddress(client);

    SDL_Log("Un joueur s'est connecté depuis %s", NET_GetAddressString(adr));

    NET_UnrefAddress(adr);

    // Recevoir le message du client et l'afficher
    // TODO(human)

    NET_DestroyStreamSocket(client);
    NET_DestroyServer(serveur);

    return true;
}

// ---------------------------------------------------------------------------
// Côté CLIENT : trouver l'hôte, s'y connecter, envoyer un message
// ---------------------------------------------------------------------------
static bool lancerClient(const char* nomHote)
{
    // 1. Transformer le nom ("127.0.0.1", "localhost", "monpc.local"...) en adresse.
    //    Ça peut demander une requête DNS : SDL_net la fait en arrière-plan...
    NET_Address* adresse = NET_ResolveHostname(nomHote);
    if (!adresse) {
        SDL_Log("Adresse invalide : %s", SDL_GetError());
        return false;
    }
    // ...et on attend qu'elle soit finie (-1 = attendre aussi longtemps qu'il faut)
    if (NET_WaitUntilResolved(adresse, -1) != NET_SUCCESS) {
        SDL_Log("Impossible de trouver « %s » : %s", nomHote, SDL_GetError());
        NET_UnrefAddress(adresse);
        return false;
    }

    // 2. Demander la connexion à l'hôte, sur son port
    SDL_Log("Connexion à %s, port %d...", NET_GetAddressString(adresse), PORT);
    NET_StreamSocket* socket = NET_CreateClient(adresse, PORT, 0);
    if (!socket || NET_WaitUntilConnected(socket, -1) != NET_SUCCESS) {
        SDL_Log("Connexion impossible : %s", SDL_GetError());
        if (socket) NET_DestroyStreamSocket(socket);
        NET_UnrefAddress(adresse);
        return false;
    }
    SDL_Log("Connecté à l'hôte !");

    // 3. Envoyer un message : ce ne sont que des octets (ici, les lettres du texte)
    const char* message = "Salut l'hôte, c'est le client !";
    NET_WriteToStreamSocket(socket, message, static_cast<int>(SDL_strlen(message)));

    // 4. S'assurer que tout est vraiment parti avant de raccrocher
    NET_WaitUntilStreamSocketDrained(socket, -1);

    NET_DestroyStreamSocket(socket);
    NET_UnrefAddress(adresse);
    return true;
}

int main(int argc, char* argv[])
{
    const bool hote   = (argc == 2 && SDL_strcmp(argv[1], "hote") == 0);
    const bool client = (argc == 3 && SDL_strcmp(argv[1], "client") == 0);
    if (!hote && !client) {
        SDL_Log("Utilisation : %s hote   OU   %s client <adresse>", argv[0], argv[0]);
        return 1;
    }

    if (!NET_Init()) {
        SDL_Log("NET_Init a échoué : %s", SDL_GetError());
        return 1;
    }

    const bool ok = hote ? lancerHote() : lancerClient(argv[2]);

    NET_Quit();
    return ok ? 0 : 1;
}
