// Leçon 6, étape 2 : un PROTOCOLE pour découper le flux TCP en messages.
// Chaque message est précédé de sa taille sur 2 octets (ordre réseau :
// octet de poids fort en premier). Plus de messages collés ni d'interblocage.
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
// Protocole : [taille sur 2 octets][données]
// ---------------------------------------------------------------------------

// Taille maximale des données d'un message (sans l'en-tête de 2 octets)
constexpr int TAILLE_MAX_MESSAGE = 1024;

// Envoie un message : d'abord sa taille sur 2 octets (poids fort en premier),
// puis les données. Renvoie false si la connexion est cassée ou si c'est trop long.
static bool envoyerMessage(NET_StreamSocket* socket, const void* donnees, int taille)
{
    if (taille < 0 || taille > TAILLE_MAX_MESSAGE) {
        SDL_Log("Taille du message négatif ou trop grand: %d", taille);
        return false;
    }

    Uint8 entete[2] {static_cast<Uint8>(taille >> 8), static_cast<Uint8>(taille & 0xFF)};

    if(!NET_WriteToStreamSocket(socket, entete, sizeof(entete))) return false;
    if(!NET_WriteToStreamSocket(socket, donnees, taille)) return false;

    return true;
}

// Octets reçus mais pas encore découpés en messages complets
struct Reception {
    Uint8 octets[4 * (2 + TAILLE_MAX_MESSAGE)];
    int   nb = 0;  // nombre d'octets valides au début de "octets"
};

// Si "reception" contient au moins un message COMPLET, le copie dans dest,
// le retire de reception, et renvoie sa taille.
// Renvoie -1 si le message n'est pas encore complet (il faut recevoir la suite),
// et -2 si le message annoncé est trop gros pour dest (protocole non respecté).
static int extraireMessage(Reception& reception, void* dest, int tailleMax)
{
    if (reception.nb < 2) return -1;

    int taille = (reception.octets[0] << 8) | reception.octets[1];
    if (taille > tailleMax) return -2;
    else if (taille + 2 > reception.nb) return -1;
    
    SDL_memcpy(dest, reception.octets + 2, taille);
    SDL_memmove(reception.octets, reception.octets + (2 + taille), reception.nb - (2 + taille));
 
    reception.nb -= taille + 2;
    return taille;
} 

// Attend puis ajoute à "reception" les octets arrivés. False si la connexion est cassée.
static bool recevoirOctets(NET_StreamSocket* socket, Reception& reception)
{
    void* aSurveiller[] = { socket };
    NET_WaitUntilInputAvailable(aSurveiller, 1, -1);

    const int placeLibre = static_cast<int>(sizeof(reception.octets)) - reception.nb;
    if (placeLibre == 0) {
        SDL_Log("Tampon de réception plein : message trop gros ?");
        return false;
    }
    const int n = NET_ReadFromStreamSocket(socket, reception.octets + reception.nb, placeLibre);
    if (n < 0) {
        SDL_Log("Connexion perdue : %s", SDL_GetError());
        return false;
    }
    reception.nb += n;
    return true;
}

// Renvoie le prochain message complet (sa taille), en attendant autant qu'il faut.
// -1 si la connexion est cassée.
static int recevoirMessage(NET_StreamSocket* socket, Reception& reception, void* dest, int tailleMax)
{
    for (;;) {
        const int taille = extraireMessage(reception, dest, tailleMax);
        if (taille >= 0) return taille;               // un message complet était déjà là
        if (taille == -2) {
            SDL_Log("Message invalide (trop gros) : on coupe la connexion");
            return -1;
        }
        if (!recevoirOctets(socket, reception)) return -1;  // sinon, attendre la suite
    }
}

// Versions "texte", construites par-dessus le protocole
static bool envoyerTexte(NET_StreamSocket* socket, const char* texte)
{
    return envoyerMessage(socket, texte, static_cast<int>(SDL_strlen(texte)));
}

static bool recevoirTexte(NET_StreamSocket* socket, Reception& reception, char* tampon, int tailleTampon)
{
    const int n = recevoirMessage(socket, reception, tampon, tailleTampon - 1);
    if (n < 0) return false;
    tampon[n] = '\0';
    return true;
}

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

    // EXPÉRIENCE : le client envoie 3 messages, on appelle 3 fois recevoirTexte
    char tampon[256];
    Reception reception;  // un tampon de réception par connexion
    for (int i = 1; i <= 3; ++i) {
        if (recevoirTexte(client, reception, tampon, sizeof(tampon)))
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
    Reception reception;
    if (recevoirTexte(socket, reception, tampon, sizeof(tampon)))
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
