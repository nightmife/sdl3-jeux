#include "reseau.h"

// Envoie un message : d'abord sa taille sur 2 octets (poids fort en premier),
// puis les données. Renvoie false si la connexion est cassée ou si c'est trop long.
bool envoyerMessage(NET_StreamSocket* socket, const void* donnees, int taille)
{
    if (taille < 0 || taille > TAILLE_MAX_MESSAGE) {
        SDL_Log("Taille du message négatif ou trop grand: %d", taille);
        return false;
    }

    // En-tête = la taille sur 2 octets, poids FORT d'abord (« ordre réseau »,
    // big-endian) : >> 8 ramène l'octet de poids fort à droite, & 0xFF garde
    // les 8 bits de droite. Les accolades interdisent les conversions avec
    // perte, d'où les static_cast explicites (on VEUT ne garder que 8 bits).
    Uint8 entete[2] {static_cast<Uint8>(taille >> 8), static_cast<Uint8>(taille & 0xFF)};

    // Envoyer l'en-tête (EXACTEMENT 2 octets : en envoyer plus lirait la mémoire
    // au-delà du tableau), puis les données. Deux envois ne posent pas de
    // problème : TCP colle tout, le receveur verra [taille][données].
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
    // 1. En-tête complet ? Sinon, attendre la suite.
    if (reception.nb < 2) return -1;

    // 2. Relire la taille (calcul inverse de l'envoi), puis vérifier :
    //    - JAMAIS confiance à une taille reçue : trop gros pour dest = -2 ;
    //    - données pas encore toutes arrivées (message coupé) = -1.
    int taille = (reception.octets[0] << 8) | reception.octets[1];
    if (taille > tailleMax) return -2;
    else if (taille + 2 > reception.nb) return -1;
    
    // 3. Copier le message (après l'en-tête) vers dest
    SDL_memcpy(dest, reception.octets + 2, taille);
    // 4. Retirer ce message du tampon : ramener au début ce qui suit (peut-être
    //    le début du message suivant, collé). memmove et pas memcpy : source et
    //    destination sont dans le même tableau et se chevauchent.
    SDL_memmove(reception.octets, reception.octets + (2 + taille), reception.nb - (2 + taille));
 
    reception.nb -= taille + 2;  // le message occupait l'en-tête (2) + les données
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
    // Ajouter à la SUITE de ce qui est déjà dans le tampon (un message peut
    // arriver en plusieurs morceaux). NET_ReadFromStreamSocket n'attend jamais :
    // elle renvoie 0 s'il n'y a rien, -1 si la connexion est cassée.
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
