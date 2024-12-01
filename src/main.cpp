#include <iostream>
#include <windows.h>
#include <conio.h>
#include "../include/J2534.h"

int main() {
   J2534 j2534;
   unsigned long deviceId;
   unsigned long channelId;

   // Init
   if (!j2534.init()) {
       std::cout << "Error: J2534 init failed" << std::endl;
       std::cout << "\nPress any key to exit...";
       _getch();
       return 1;
   }

   // Open
   if (j2534.PassThruOpen(NULL, &deviceId)) {
       std::cout << "Error: Can't open interface" << std::endl;
       std::cout << "\nPress any key to exit...";
       _getch();
       return 1;
   }

   // Connect ISO15765 500K
   if (j2534.PassThruConnect(deviceId, ISO15765, ISO15765_FRAME_PAD, 500000, &channelId)) {
       std::cout << "Error: Connection failed" << std::endl;
       j2534.PassThruClose(deviceId);
       std::cout << "\nPress any key to exit...";
       _getch();
       return 1;
   }

   std::cout << "Reading DTCs - Press any key to exit" << std::endl;

   while (!_kbhit()) {
       // Mode 03 request
       PASSTHRU_MSG msg;
       memset(&msg, 0, sizeof(PASSTHRU_MSG));
       msg.ProtocolID = ISO15765;
       msg.TxFlags = ISO15765_FRAME_PAD;
       msg.Data[0] = 0x00;
       msg.Data[1] = 0x00;
       msg.Data[2] = 0x07;
       msg.Data[3] = 0xDF;
       msg.Data[4] = 0x03;
       msg.DataSize = 5;

       unsigned long numMsg = 1;
       j2534.PassThruWriteMsgs(channelId, &msg, &numMsg, 0);

       // Read response
       Sleep(100);

       PASSTHRU_MSG rxMsg;
       memset(&rxMsg, 0, sizeof(PASSTHRU_MSG));
       numMsg = 1;
   
       if (j2534.PassThruReadMsgs(channelId, &rxMsg, &numMsg, 1000) == 0 && numMsg > 0) {
           std::cout << "Response: ";
           for(unsigned int i = 0; i < rxMsg.DataSize; i++) {
               printf("%02X ", rxMsg.Data[i]);
           }
           std::cout << std::endl;
       }

       Sleep(1000); // Pause 1 second between requests
   }

   // Cleanup
   j2534.PassThruDisconnect(channelId);
   j2534.PassThruClose(deviceId);

   return 0;
}