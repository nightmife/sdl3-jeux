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
// Outils communs aux deux côtés
// ---------------------------------------------------------------------------

// Envoie le texte (sans le '\0' final). Renvoie false si la connexion est cassée.
static bool envoyerTexte(NET_StreamSocket* socket, const char* texte)
{
    // strlen ne compte pas le '\0' : on envoie juste les lettres
    return NET_WriteToStreamSocket(socket, texte, static_cast<int>(SDL_strlen(texte)));
}

// Attend un message, le copie dans tampon (qui peut contenir tailleTampon octets)
// et le termine par '\0'. Renvoie true si un message a été reçu.
static bool recevoirTexte(NET_StreamSocket* socket, char* tampon, int tailleTampon)
{
    void *aSurveillerClient[] = { socket };
    NET_WaitUntilInputAvailable(aSurveillerClient, 1, -1);
    
    // n = nombre d'octets reçus (0 = rien pour l'instant, -1 = connexion cassée).
    // On lit au plus tailleTampon - 1 octets pour garder une case pour le '\0'.
    // (tailleTampon est fourni par l'appelant : ici, tampon est un POINTEUR,
    //  et sizeof(tampon) vaudrait 8, pas la taille du tableau !)
    int n = NET_ReadFromStreamSocket(socket, tampon, tailleTampon - 1);
   
    if (n > 0) {
         tampon[n] = '\0';  // les octets reçus deviennent une vraie chaîne C
         return true;
    } else if (n < 0) SDL_Log("Connexion perdu: %s", SDL_GetError());
    return false;
}

// ---------------------------------------------------------------------------
// Côté HÔTE : ouvrir la salle, attendre un client, l'accepter
// ---------------------------------------------------------------------------
static bool lancerHote()
{
    // Ouvrir la "salle" : écouter sur le port PORT (nullptr = toutes les
    // adresses de la machine). Le serveur ne sert qu'à DÉCROCHER, jamais à parler.
    NET_Server *serveur = NET_CreateServer(nullptr, PORT, 0);
    if (serveur == nullptr) {
        SDL_Log("Impossible d'ouvrir la salle: %s", SDL_GetError());

        return false;
    }
    SDL_Log("Salle ouverte sur le port %d", PORT);

    // Attendre qu'un client frappe à la porte (-1 = sans limite de temps).
    // Bloquant : acceptable dans un programme terminal, INTERDIT dans le jeu.
    void *aSurveiller[] = { serveur };
    NET_WaitUntilInputAvailable(aSurveiller, 1, -1);

    // Décrocher : c'est NET_AcceptClient qui CRÉE le socket du client (on lui
    // passe l'adresse de notre pointeur pour qu'il le remplisse).
    // Deux cas d'échec : une erreur (false), ou finalement personne (nullptr).
    NET_StreamSocket *client = nullptr;
    if (!NET_AcceptClient(serveur, &client) || client == nullptr) {
        SDL_Log("Aucun joueur");
        
        NET_DestroyServer(serveur);

        return false;
    }

    // Adresse du client. La doc impose de la libérer avec NET_UnrefAddress.
    NET_Address *adr = NET_GetStreamSocketAddress(client);

    SDL_Log("Un joueur s'est connecté depuis %s", NET_GetAddressString(adr));

    NET_UnrefAddress(adr);

    // EXPÉRIENCE : le client envoie 3 messages, on appelle 3 fois recevoirTexte
    char tampon[256];
    for (int i = 1; i <= 3; ++i) {
        if (recevoirTexte(client, tampon, sizeof(tampon)))
            SDL_Log("Lecture n°%d : [%s]", i, tampon);
    }
    envoyerTexte(client, "Bienvenue dans la salle !");
    NET_WaitUntilStreamSocketDrained(client, -1);  // laisser partir la réponse avant de raccrocher

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

    // 3. EXPÉRIENCE : envoyer 3 messages d'affilée, puis attendre la réponse de l'hôte
    envoyerTexte(socket, "Bonjour");
    envoyerTexte(socket, "Je suis le joueur 2");
    envoyerTexte(socket, "On joue ?");
    char tampon[256];
    if (recevoirTexte(socket, tampon, sizeof(tampon)))
        SDL_Log("Réponse de l'hôte : %s", tampon);

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
