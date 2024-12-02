#include <iostream>
#include <vector>
#include <cstring>
#include <sstream>
#include <windows.h>
#include "../include/J2534.h"

unsigned long deviceID;
unsigned long channelID;
unsigned long filterID;

// Function to log the last error
void logLastError(J2534& j2534) {
    char errorDescription[256] = {0};
    j2534.PassThruGetLastError(errorDescription);
    std::cerr << "Last error: " << errorDescription << std::endl;
}

// Function to configure a CAN filter
bool configureCanFilter(J2534& j2534) {
    PASSTHRU_MSG mask, pattern;

    // Complete initialization with memset
    memset(&mask, 0, sizeof(PASSTHRU_MSG));
    memset(&pattern, 0, sizeof(PASSTHRU_MSG));

    // Configure the mask
    mask.ProtocolID = ISO15765;
    mask.DataSize = 4;
    memset(mask.Data, 0xFF, sizeof(mask.Data)); // Match all bits

    // Configure the pattern
    pattern.ProtocolID = ISO15765;
    pattern.DataSize = 4;
    pattern.Data[0] = 0x07; // High byte of ID (0x7E8)
    pattern.Data[1] = 0xE8; // Low byte of ID (0x7E8)
    pattern.Data[2] = 0x00;
    pattern.Data[3] = 0x00;

    // Add the filter
    long status = j2534.PassThruStartMsgFilter(channelID, PASS_FILTER, &mask, &pattern, nullptr, &filterID);
    if (status != STATUS_NOERROR) {
        logLastError(j2534);
        std::cerr << "Error: Unable to configure the CAN filter." << std::endl;
        return false;
    }

    std::cout << "CAN filter configured successfully (Filter ID: " << filterID << ")." << std::endl;
    return true;
}

// Function to send a CAN message and read the raw response
std::string sendPassThruMsg(J2534& j2534, const std::vector<unsigned char>& frame) {
    PASSTHRU_MSG request;
    memset(&request, 0, sizeof(PASSTHRU_MSG));
    request.ProtocolID = ISO15765;
    request.TxFlags = 0; // No special flags
    request.DataSize = frame.size();
    memcpy(request.Data, frame.data(), frame.size());

    unsigned long numMsgs = 1;
    long status = j2534.PassThruWriteMsgs(channelID, &request, &numMsgs, 1000);
    if (status != STATUS_NOERROR) {
        logLastError(j2534);
        return "Error: Failed to send message (Code: " + std::to_string(status) + ")";
    }

    PASSTHRU_MSG response[1];
    unsigned long numResponses = 1;
    status = j2534.PassThruReadMsgs(channelID, response, &numResponses, 2000);
    if (status != STATUS_NOERROR || numResponses == 0) {
        logLastError(j2534);
        return "Error: No response received (Code: " + std::to_string(status) + ")";
    }

    std::ostringstream rawResponseStream;
    for (unsigned long i = 0; i < numResponses; ++i) {
        rawResponseStream << "Response " << i + 1 << ": ";
        for (unsigned long j = 0; j < response[i].DataSize; ++j) {
            rawResponseStream << std::uppercase << std::hex << (int)response[i].Data[j] << " ";
        }
        rawResponseStream << std::endl;
    }

    return rawResponseStream.str();
}

int main() {
    J2534 j2534;

    // Initialize the J2534 library
    if (!j2534.init()) {
        std::cerr << "Error: Failed to initialize the J2534 library." << std::endl;
        return 1;
    }

    // Open the J2534 interface
    if (j2534.PassThruOpen(nullptr, &deviceID) != STATUS_NOERROR) {
        std::cerr << "Error: Unable to open the J2534 interface." << std::endl;
        logLastError(j2534);
        return 1;
    }

    // Connect to the CAN protocol
    if (j2534.PassThruConnect(deviceID, ISO15765, ISO15765_FRAME_PAD, 500000, &channelID) != STATUS_NOERROR) {
        std::cerr << "Error: Failed to connect to the CAN protocol." << std::endl;
        logLastError(j2534);
        j2534.PassThruClose(deviceID);
        return 1;
    }

    // Configure the CAN filter
    if (!configureCanFilter(j2534)) {
        j2534.PassThruDisconnect(channelID);
        j2534.PassThruClose(deviceID);
        return 1;
    }

    std::cout << "CAN connection established. Sending request..." << std::endl;

    // Mode 03 request (Read DTCs)
    std::vector<unsigned char> mode03 = { 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    std::string rawResponse = sendPassThruMsg(j2534, mode03);

    std::cout << "Raw CAN response:" << std::endl << rawResponse;

    // Clean up and close connections
    if (j2534.PassThruStopMsgFilter(channelID, filterID) != STATUS_NOERROR) {
        std::cerr << "Error: Unable to remove the CAN filter." << std::endl;
        logLastError(j2534);
    }

    if (j2534.PassThruDisconnect(channelID) != STATUS_NOERROR) {
        std::cerr << "Error: Unable to disconnect the channel." << std::endl;
        logLastError(j2534);
    }

    if (j2534.PassThruClose(deviceID) != STATUS_NOERROR) {
        std::cerr << "Error: Unable to close the J2534 interface." << std::endl;
        logLastError(j2534);
    }

    std::cout << "Connection closed. Program terminated." << std::endl;
    std::cin.get();
    return 0;
}
