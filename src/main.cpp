#include <iostream>
#include <windows.h>
#include <conio.h>
#include "../include/J2534.h" // Assurez-vous que le chemin est correct

// Définition des RxStatus flags
#define TX_MSG_TYPE      0x00000001
#define START_OF_MESSAGE 0x00000002

int main() {
    J2534 j2534;
    unsigned long deviceId;
    unsigned long channelId;

    // Initialisation
    if (!j2534.init()) {
        std::cout << "Erreur : J2534 init échoué" << std::endl;
        std::cout << "\nAppuyez sur une touche pour quitter...";
        _getch();
        return 1;
    }

    // Ouvrir l'interface J2534
    if (j2534.PassThruOpen(nullptr, &deviceId)) {
        std::cout << "Erreur : Impossible d'ouvrir l'interface" << std::endl;
        std::cout << "\nAppuyez sur une touche pour quitter...";
        _getch();
        return 1;
    }

    // Connexion au protocole ISO15765 (500 kbps)
    if (j2534.PassThruConnect(deviceId, ISO15765, ISO15765_FRAME_PAD, 500000, &channelId)) {
        std::cout << "Erreur : Connexion échouée" << std::endl;
        j2534.PassThruClose(deviceId);
        std::cout << "\nAppuyez sur une touche pour quitter...";
        _getch();
        return 1;
    }

    std::cout << "Lecture des DTCs (Mode 03) - Appuyez sur une touche pour quitter." << std::endl;

    while (!_kbhit()) { // Boucle tant qu'aucune touche n'est appuyée
        // Préparation du message Mode 03
        PASSTHRU_MSG msg;
        memset(&msg, 0, sizeof(PASSTHRU_MSG));
        msg.ProtocolID = ISO15765;
        msg.TxFlags = ISO15765_FRAME_PAD;
        msg.Data[0] = 0x01; // Longueur du message
        msg.Data[1] = 0x03; // Mode 03 - Lire les DTC
        msg.Data[2] = 0x00;
        msg.Data[3] = 0x00;
        msg.Data[4] = 0x00;
        msg.Data[5] = 0x00;
        msg.Data[6] = 0x00;
        msg.Data[7] = 0x00;
        msg.DataSize = 8;

        unsigned long numMsg = 1;
        if (j2534.PassThruWriteMsgs(channelId, &msg, &numMsg, 1000) != STATUS_NOERROR) {
            std::cerr << "Erreur : Échec de l'envoi de la requête." << std::endl;
        }

        // Lire la réponse
        Sleep(100); // Attente pour laisser le temps à l'ECU de répondre

        PASSTHRU_MSG rxMsg;
        memset(&rxMsg, 0, sizeof(PASSTHRU_MSG));
        numMsg = 1;

        if (j2534.PassThruReadMsgs(channelId, &rxMsg, &numMsg, 1000) == STATUS_NOERROR && numMsg > 0) {
            // Filtrer les messages de réponse (pas TX ou START_OF_MESSAGE)
            if (!(rxMsg.RxStatus & TX_MSG_TYPE) && !(rxMsg.RxStatus & START_OF_MESSAGE)) {
                std::cout << "Réponse : ";
                for (unsigned int i = 0; i < rxMsg.DataSize; i++) {
                    printf("%02X ", rxMsg.Data[i]);
                }
                std::cout << std::endl;
            }
        } else {
            std::cerr << "Erreur : Aucune réponse reçue." << std::endl;
        }

        Sleep(1000); // Pause de 1 seconde entre les requêtes
    }

    // Nettoyage
    j2534.PassThruDisconnect(channelId);
    j2534.PassThruClose(deviceId);

    return 0;
}
