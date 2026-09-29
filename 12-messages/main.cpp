// Leçon 6, étape 3 : envoyer des DONNÉES du jeu (les Entrees), pas du texte.
//   ./messages hote              -> reçoit et affiche les entrées d'un client
//   ./messages client <adresse>  -> envoie une série d'entrées de test

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_net/SDL_net.h>

#include "messages.h"
#include "reseau.h"

static void afficherEntrees(const char* prefixe, const Entrees& e)
{
    SDL_Log("%s haut=%d bas=%d gauche=%d droite=%d", prefixe, e.haut, e.bas, e.gauche, e.droite);
}

static bool lancerHote()
{
    NET_Server* serveur = NET_CreateServer(nullptr, PORT, 0);
    if (!serveur) {
        SDL_Log("Impossible d'ouvrir la salle : %s", SDL_GetError());
        return false;
    }
    SDL_Log("Salle ouverte sur le port %d, en attente d'un joueur...", PORT);

    void* aSurveiller[] = { serveur };
    NET_WaitUntilInputAvailable(aSurveiller, 1, -1);
    NET_StreamSocket* client = nullptr;
    if (!NET_AcceptClient(serveur, &client) || !client) {
        SDL_Log("Aucun joueur : %s", SDL_GetError());
        NET_DestroyServer(serveur);
        return false;
    }
    SDL_Log("Un joueur est connecté !");

    // Recevoir des messages jusqu'à ce que le client raccroche
    Reception reception;
    std::uint8_t message[TAILLE_MAX_MESSAGE];
    int n;
    while ((n = recevoirMessage(client, reception, message, sizeof(message))) >= 0) {
        Entrees e;
        if (decoderEntrees(message, n, e))
            afficherEntrees("Reçu :", e);
        else
            SDL_Log("Message inconnu ou invalide (%d octets)", n);
    }
    SDL_Log("Le joueur est parti.");

    NET_DestroyStreamSocket(client);
    NET_DestroyServer(serveur);
    return true;
}

static bool lancerClient(const char* nomHote)
{
    NET_Address* adresse = NET_ResolveHostname(nomHote);
    if (!adresse || NET_WaitUntilResolved(adresse, -1) != NET_SUCCESS) {
        SDL_Log("Impossible de trouver « %s » : %s", nomHote, SDL_GetError());
        if (adresse) NET_UnrefAddress(adresse);
        return false;
    }
    NET_StreamSocket* socket = NET_CreateClient(adresse, PORT, 0);
    if (!socket || NET_WaitUntilConnected(socket, -1) != NET_SUCCESS) {
        SDL_Log("Connexion impossible : %s", SDL_GetError());
        if (socket) NET_DestroyStreamSocket(socket);
        NET_UnrefAddress(adresse);
        return false;
    }
    SDL_Log("Connecté à l'hôte !");

    // Une série de combinaisons de touches à envoyer
    const Entrees essais[] = {
        {true,  false, false, false},  // haut
        {true,  false, false, true },  // haut + droite
        {false, false, false, false},  // rien
        {false, true,  true,  true },  // bas + gauche + droite
        {true,  true,  true,  true },  // tout
    };
    for (const Entrees& e : essais) {
        std::uint8_t message[TAILLE_MESSAGE_ENTREES];
        const int taille = encoderEntrees(e, message);
        afficherEntrees("Envoi :", e);
        envoyerMessage(socket, message, taille);
    }

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
