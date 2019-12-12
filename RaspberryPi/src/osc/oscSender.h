#ifndef OSCSENDER_H
#define OSCSENDER_H

#include <string>
#include <iostream>
#include <cstring>
#include <sstream>

#if defined(__BORLANDC__) // workaround for BCB4 release build intrinsics bug
namespace std {
using ::__strcmp__;  // avoid error: E2316 '__strcmp__' is not a member of 'std'.
}
#endif

#include <lib/oscpack/osc/OscOutboundPacketStream.h>
#include <lib/oscpack/ip/UdpSocket.h>

#define ADDRESS "MacBoi.local"
#define PORT 8000
#define PRINT_PACKETS true
#define OUTPUT_BUFFER_SIZE 1024

class OSCSender {
public:
    OSCSender() {

    }

    void send(std::string address, int value) {

        UdpTransmitSocket transmitSocket = UdpTransmitSocket ( IpEndpointName( ADDRESS, PORT ) );

        char buffer[300];

        osc::OutboundPacketStream p( buffer, 300);
        p << osc::BeginBundleImmediate
          << osc::BeginMessage( address.c_str() );
        p << value;
        p << osc::EndMessage << osc::EndBundle;

        transmitSocket.Send( p.Data(), p.Size() );
        p.Clear();

        std::cout << "Sent some OSC of size " << 300 << std::endl;

    }
private:
    ;

};


#endif