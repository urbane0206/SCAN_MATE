#include <iostream>
#include <vector>
#include <cstring>
#include <sstream>
#include <windows.h>
#include "../include/J2534.h" // Inclure la bibliothèque J2534

unsigned long deviceID;
unsigned long channelID;

// Fonction pour convertir un tableau de bytes en chaîne hexadécimale
std::string bytesToHex(const std::vector<unsigned char>& data) {
    std::ostringstream hexStream;
    for (unsigned char byte : data) {
        hexStream << std::uppercase << std::hex << (int)byte << " ";
    }
    return hexStream.str();
}

// Fonction pour convertir un entier en chaîne (alternative à std::to_string)
template <typename T>
std::string to_string_alternative(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// Fonction pour envoyer un message CAN (PassThruWriteMsgs) et lire la réponse
std::string sendPassThruMsg(J2534& j2534, const std::vector<unsigned char>& frame) {
    PASSTHRU_MSG request;
    memset(&request, 0, sizeof(PASSTHRU_MSG));
    request.ProtocolID = ISO15765;
    request.TxFlags = ISO15765_FRAME_PAD; // Remplissage automatique des trames
    request.DataSize = frame.size();
    memcpy(request.Data, frame.data(), frame.size());

    unsigned long numMsgs = 1;
    long status = j2534.PassThruWriteMsgs(channelID, &request, &numMsgs, 1000);
    if (status != STATUS_NOERROR) {
        return "Erreur lors de l'envoi du message (Code : " + to_string_alternative(status) + ")";
    }

    PASSTHRU_MSG response[10];
    unsigned long numResponses = 10;
    status = j2534.PassThruReadMsgs(channelID, response, &numResponses, 2000);
    if (status != STATUS_NOERROR || numResponses == 0) {
        return "Aucune réponse reçue (Code : " + to_string_alternative(status) + ")";
    }

    std::vector<unsigned char> responseData;
    for (unsigned long i = 0; i < numResponses; ++i) {
        responseData.insert(responseData.end(), response[i].Data, response[i].Data + response[i].DataSize);
    }

    return bytesToHex(responseData);
}

int main() {
    J2534 j2534;

    // Initialiser la bibliothèque J2534
    if (!j2534.init()) {
        std::cerr << "Erreur : Échec de l'initialisation de la bibliothèque J2534." << std::endl;
        return 1;
    }

    // Ouvrir l'interface J2534
    if (j2534.PassThruOpen(nullptr, &deviceID) != STATUS_NOERROR) {
        std::cerr << "Erreur : Impossible d'ouvrir l'interface J2534." << std::endl;
        std::cout << "\nAppuyez sur une touche pour quitter...";
        std::cin.get();
        return 1;
    }

    // Connecter au protocole CAN
    if (j2534.PassThruConnect(deviceID, ISO15765, ISO15765_FRAME_PAD, 500000, &channelID) != STATUS_NOERROR) {
        std::cerr << "Erreur : Connexion au protocole CAN échouée." << std::endl;
        std::cout << "\nAppuyez sur une touche pour quitter...";
        std::cin.get();
        j2534.PassThruClose(deviceID);
        return 1;
    }

    std::cout << "Connexion CAN établie. Lecture des codes défauts..." << std::endl;

    // Message Mode 03 (Lire les DTCs)
    std::vector<unsigned char> mode03 = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    std::string response = sendPassThruMsg(j2534, mode03);

    std::cout << "Réponse CAN : " << response << std::endl;

    // Nettoyer et fermer les connexions
    j2534.PassThruDisconnect(channelID);
    j2534.PassThruClose(deviceID);

    std::cout << "Connexion fermée. Fin du programme." << std::endl;
    std::cout << "\nAppuyez sur une touche pour quitter...";
    std::cin.get(); // Remplacement de _getch() pour plus de compatibilité
    return 1;
}
