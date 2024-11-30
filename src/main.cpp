#include <iostream>
#include <windows.h>
#include <conio.h>
#include "J2534.h"

const int TIMEOUT = 1000; // 1 second

int main() {
    J2534 j2534;
    unsigned long deviceId;
    unsigned long channelId;

    // Initialize J2534 library
    if (!j2534.init()) {
        std::cerr << "Error: Unable to load J2534 DLL.\n";
        std::cout << "Press any key to exit...\n";
        _getch();
        return 1;
    }

    // Open the device
    if (j2534.PassThruOpen(nullptr, &deviceId) != STATUS_NOERROR) {
        std::cerr << "Error: Unable to open the device.\n";
        std::cout << "Press any key to exit...\n";
        _getch();
        return 1;
    }

    // Connect to ISO15765 (CAN) protocol
    if (j2534.PassThruConnect(deviceId, ISO15765, CAN_ID_BOTH, 500000, &channelId) != STATUS_NOERROR) {
        std::cerr << "Error: CAN connection failed.\n";
        j2534.PassThruClose(deviceId);
        std::cout << "Press any key to exit...\n";
        _getch();
        return 1;
    }

    // Set up filter to capture DTC responses
    PASSTHRU_MSG mask = {}, pattern = {};
    mask.ProtocolID = ISO15765;
    pattern.ProtocolID = ISO15765;
    mask.DataSize = pattern.DataSize = 4;
    std::fill(std::begin(mask.Data), std::begin(mask.Data) + 4, 0xFF);
    pattern.Data[0] = 0x00;
    pattern.Data[1] = 0x00;
    pattern.Data[2] = 0x07;
    pattern.Data[3] = 0xE8; // ECU response ID

    unsigned long filterId;
    if (j2534.PassThruStartMsgFilter(channelId, PASS_FILTER, &mask, &pattern, nullptr, &filterId) != STATUS_NOERROR) {
        std::cerr << "Error: Filter configuration failed.\n";
        j2534.PassThruDisconnect(channelId);
        j2534.PassThruClose(deviceId);
        std::cout << "Press any key to exit...\n";
        _getch();
        return 1;
    }

    // Send request to read DTCs (Mode 03)
    PASSTHRU_MSG txMsg = {};
    txMsg.ProtocolID = ISO15765;
    txMsg.TxFlags = ISO15765_FRAME_PAD;
    txMsg.DataSize = 8;
    txMsg.Data[0] = 0x01; // Request length
    txMsg.Data[1] = 0x03; // Mode 03: Read DTCs
    for (int i = 2; i < 8; ++i) {
        txMsg.Data[i] = 0x00;
    }

    unsigned long numMsgs = 1;
    if (j2534.PassThruWriteMsgs(channelId, &txMsg, &numMsgs, TIMEOUT) != STATUS_NOERROR) {
        std::cerr << "Error: Failed to send request.\n";
        j2534.PassThruDisconnect(channelId);
        j2534.PassThruClose(deviceId);
        std::cout << "Press any key to exit...\n";
        _getch();
        return 1;
    }

    // Read responses
    PASSTHRU_MSG rxMsg = {};
    numMsgs = 1;
    if (j2534.PassThruReadMsgs(channelId, &rxMsg, &numMsgs, TIMEOUT) == STATUS_NOERROR && numMsgs > 0) {
        std::cout << "Response received: ";
        for (unsigned int i = 0; i < rxMsg.DataSize; ++i) {
            printf("0x%02X ", rxMsg.Data[i]);
        }
        std::cout << "\n";
    } else {
        std::cerr << "Error: No response received.\n";
    }

    // Close connections
    j2534.PassThruDisconnect(channelId);
    j2534.PassThruClose(deviceId);

    // Wait for a key press before exiting
    std::cout << "Press any key to exit...\n";
    _getch();

    return 0;
}
