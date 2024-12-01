#include <iostream>
#include <windows.h>
#include <conio.h>
#include "../include/J2534.h"

int main() {
    J2534 j2534;

    if (!j2534.init()) {
        std::cout << "Error: Unable to initialize J2534" << std::endl;
        std::cout << "\nPress any key to exit...";
        _getch();
        return 1;
    }

    unsigned long deviceId;
    unsigned long channelId;

    // Open interface
    if (j2534.PassThruOpen(NULL, &deviceId)) {
        std::cout << "Error opening interface" << std::endl;
        std::cout << "\nPress any key to exit...";
        _getch();
        return 1;
    }

    // Connect to ISO15765 protocol at 500kbps
    if (j2534.PassThruConnect(deviceId, ISO15765, ISO15765_FRAME_PAD, 500000, &channelId)) {
        std::cout << "Error connecting ISO15765" << std::endl;
        j2534.PassThruClose(deviceId);
        std::cout << "\nPress any key to exit...";
        _getch();
        return 1;
    }

    // Build mode 03 request message
    PASSTHRU_MSG reqMsg;
    memset(&reqMsg, 0, sizeof(reqMsg));
    reqMsg.ProtocolID = ISO15765;
    reqMsg.RxStatus = 0;
    reqMsg.TxFlags = ISO15765_FRAME_PAD;
    reqMsg.Timestamp = 0;
    reqMsg.DataSize = 4;      
    reqMsg.ExtraDataIndex = 0;
    reqMsg.Data[0] = 0x01;    
    reqMsg.Data[1] = 0x03;
    reqMsg.Data[3] = 0x00;
    reqMsg.Data[4] = 0x00;


    

    // Send the request
    unsigned long numMsg = 1;
    if (j2534.PassThruWriteMsgs(channelId, &reqMsg, &numMsg, 1000)) {
        std::cout << "Error sending message" << std::endl;
    } else {
        std::cout << "Mode 03 request sent" << std::endl;
    }

    // Wait for response (first read)
    PASSTHRU_MSG rxMsg;
    memset(&rxMsg, 0, sizeof(rxMsg));
    numMsg = 1;
    if (!j2534.PassThruReadMsgs(channelId, &rxMsg, &numMsg, 1000)) {
        std::cout << "First response received:" << std::endl;
        printf("Raw Data: ");
        for (unsigned int i = 0; i < rxMsg.DataSize; i++) {
            printf("%02X ", rxMsg.Data[i]);
        }
        printf("\n");
    } else {
        std::cout << "No response received on first read" << std::endl;
    }

    // Delay before second read
    Sleep(100);

    // Wait for response (second read)
    memset(&rxMsg, 0, sizeof(rxMsg));
    numMsg = 1;
    if (!j2534.PassThruReadMsgs(channelId, &rxMsg, &numMsg, 1000)) {
        std::cout << "Second response received:" << std::endl;
        printf("Raw Data: ");
        for (unsigned int i = 0; i < rxMsg.DataSize; i++) {
            printf("%02X ", rxMsg.Data[i]);
        }
        printf("\n");
    } else {
        std::cout << "No response received on second read" << std::endl;
    }

    // Close connection and interface
    j2534.PassThruDisconnect(channelId);
    j2534.PassThruClose(deviceId);

    std::cout << "\nPress any key to exit...";
    _getch();

    return 0;
}