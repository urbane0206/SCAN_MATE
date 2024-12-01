#include <iostream>
#include <windows.h>
#include <conio.h>  // Pour _getch()
#include "../include/J2534.h"

int main() {
   J2534 j2534;
   j2534.debug(true);  // Active le debug pour voir les messages bruts
   
   unsigned long deviceId;
   unsigned long channelId;
   unsigned long filterId;

   std::cout << "Initialisation interface J2534..." << std::endl;

   // Initialisation
   if (!j2534.init()) {
       std::cout << "Erreur: Impossible d'initialiser J2534" << std::endl;
       std::cout << "\nAppuyez sur une touche pour quitter...";
       _getch();
       return 1;
   }

   // Ouverture device
   if (j2534.PassThruOpen(NULL, &deviceId)) {
       std::cout << "Erreur ouverture interface" << std::endl;
       std::cout << "\nAppuyez sur une touche pour quitter...";
       _getch();
       return 1;
   }

   std::cout << "Configuration ISO15765..." << std::endl;

   // Connexion ISO15765 à 500K
   if (j2534.PassThruConnect(deviceId, ISO15765, ISO15765_FRAME_PAD, 500000, &channelId)) {
       std::cout << "Erreur connexion CAN" << std::endl;
       j2534.PassThruClose(deviceId);
       std::cout << "\nAppuyez sur une touche pour quitter...";
       _getch();
       return 1;
   }

   // Configuration filtre (0x7DF -> 0x7E8)
   PASSTHRU_MSG maskMsg, patternMsg, flowMsg;
   memset(&maskMsg, 0, sizeof(PASSTHRU_MSG));
   memset(&patternMsg, 0, sizeof(PASSTHRU_MSG));
   memset(&flowMsg, 0, sizeof(PASSTHRU_MSG));

   // Configuration commune
   maskMsg.ProtocolID = patternMsg.ProtocolID = flowMsg.ProtocolID = ISO15765;
   maskMsg.DataSize = patternMsg.DataSize = flowMsg.DataSize = 4;

   // Masque
   for(int i = 0; i < 4; i++)
       maskMsg.Data[i] = 0xFF;

   // Pattern pour 0x7DF
   patternMsg.Data[0] = 0x00;
   patternMsg.Data[1] = 0x00;
   patternMsg.Data[2] = 0x07;
   patternMsg.Data[3] = 0xDF;

   // Flow control pour 0x7E8
   flowMsg.Data[0] = 0x00;
   flowMsg.Data[1] = 0x00;
   flowMsg.Data[2] = 0x07;
   flowMsg.Data[3] = 0xE8;

   std::cout << "Configuration filtre CAN..." << std::endl;

   // Mise en place du filtre
   if (j2534.PassThruStartMsgFilter(channelId, FLOW_CONTROL_FILTER, 
       &maskMsg, &patternMsg, &flowMsg, &filterId)) {
       std::cout << "Erreur configuration filtre" << std::endl;
       j2534.PassThruDisconnect(channelId);
       j2534.PassThruClose(deviceId);
       std::cout << "\nAppuyez sur une touche pour quitter...";
       _getch();
       return 1;
   }

   // Préparation requête Mode 03
   PASSTHRU_MSG msg;
   memset(&msg, 0, sizeof(PASSTHRU_MSG));
   msg.ProtocolID = ISO15765;
   msg.TxFlags = ISO15765_FRAME_PAD;
   
   // Message OBD Mode 03
   msg.Data[0] = 0x01;    // Longueur
   msg.Data[1] = 0x03;    // Mode 03
   msg.DataSize = 8;      // Longueur totale avec padding

   std::cout << "\nEnvoi requête Mode 03..." << std::endl;

   // Envoi avec debug actif pour voir la trame exacte
   unsigned long numMsg = 1;
   if (j2534.PassThruWriteMsgs(channelId, &msg, &numMsg, 1000)) {
       std::cout << "Erreur envoi message" << std::endl;
       j2534.PassThruDisconnect(channelId);
       j2534.PassThruClose(deviceId);
       std::cout << "\nAppuyez sur une touche pour quitter...";
       _getch();
       return 1;
   }

   std::cout << "\nAttente réponse (300ms)..." << std::endl;
   Sleep(300);  // Délai 300ms
   
   // Lecture réponse
   PASSTHRU_MSG rxMsg;
   memset(&rxMsg, 0, sizeof(PASSTHRU_MSG));
   numMsg = 1;

   if (j2534.PassThruReadMsgs(channelId, &rxMsg, &numMsg, 1000) == 0 && numMsg > 0) {
       std::cout << "Réponse reçue:" << std::endl;
       std::cout << "Taille données: " << rxMsg.DataSize << " bytes" << std::endl;
       std::cout << "Données brutes: ";
       for(unsigned int i = 0; i < rxMsg.DataSize; i++) {
           printf("0x%02X ", rxMsg.Data[i]);
       }
       std::cout << std::endl;
       
       // Si on reçoit une réponse mode 03 valide (0x43)
       if(rxMsg.Data[1] == 0x43) {
           std::cout << "\nRéponse mode 03 valide\n";
           // Le nombre de DTCs est (DataSize-2)/2 car:
           // - 2 premiers bytes = longueur et mode
           // - Chaque DTC fait 2 bytes
           int numDTCs = (rxMsg.DataSize-2)/2;
           std::cout << "Nombre de DTCs: " << numDTCs << std::endl;
       }
   } else {
       std::cout << "Pas de réponse ou erreur" << std::endl;
   }

   // Nettoyage
   j2534.PassThruDisconnect(channelId);
   j2534.PassThruClose(deviceId);

   std::cout << "\nAppuyez sur une touche pour quitter...";
   _getch();

   return 0;
}