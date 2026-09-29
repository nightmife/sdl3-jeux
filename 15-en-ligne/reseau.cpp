#include "reseau.h"

// Envoie un message : d'abord sa taille sur 2 octets (poids fort en premier),
// puis les données. Renvoie false si la connexion est cassée ou si c'est trop long.
bool envoyerMessage(NET_StreamSocket* socket, const void* donnees, int taille)
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

// Si "reception" contient au moins un message COMPLET, le copie dans dest,
// le retire de reception, et renvoie sa taille.
// Renvoie -1 si le message n'est pas encore complet (il faut recevoir la suite),
// et -2 si le message annoncé est trop gros pour dest (protocole non respecté).
int extraireMessage(Reception& reception, void* dest, int tailleMax)
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
bool recevoirOctets(NET_StreamSocket* socket, Reception& reception)
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
int recevoirMessage(NET_StreamSocket* socket, Reception& reception, void* dest, int tailleMax)
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
