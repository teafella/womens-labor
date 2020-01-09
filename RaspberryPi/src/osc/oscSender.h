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

#define ADDRESS "iron.local"
#define PORT 8000
#define PRINT_PACKETS true
#define OUTPUT_BUFFER_SIZE 1024

class OSCSender {
public:
    OSCSender() {
        transmit_socket_ = new UdpTransmitSocket( IpEndpointName( ADDRESS, PORT ) );
    }

    void send(std::string address, int value) {
        char buffer[OUTPUT_BUFFER_SIZE];
        osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE);
        p << osc::BeginBundleImmediate
          << osc::BeginMessage( address.c_str() );
        p << value;
        p << osc::EndMessage << osc::EndBundle;

         transmit_socket_->Send( p.Data(), p.Size() );
        p.Clear();

        // std::cout << "Sent some OSC of size " << OUTPUT_BUFFER_SIZE << std::endl;
    }

    void send(std::string address, int value1, float value2) {

        char buffer[OUTPUT_BUFFER_SIZE];
        osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE);
        p << osc::BeginBundleImmediate
          << osc::BeginMessage( address.c_str() );
        p << value1;
        p << value2;
        p << osc::EndMessage << osc::EndBundle;

        transmit_socket_->Send( p.Data(), p.Size() );
        p.Clear();

        // std::cout << "Sent some OSC of size " << OUTPUT_BUFFER_SIZE << std::endl;
    }
private:
    UdpTransmitSocket* transmit_socket_;
};


#endif