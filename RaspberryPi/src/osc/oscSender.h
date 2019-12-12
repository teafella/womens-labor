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
#define PORT 7000
#define PRINT_PACKETS true

#define OUTPUT_BUFFER_SIZE 1024

class OSCSender {
public:
    OSCSender() {

    }

    void send(int data_size, unsigned char* in_data, int frame_width, int frame_height ) {

        // unsigned char* last_frame_data_ = new unsigned char[data_size];
        // memcpy(last_frame_data_, in_data, data_size * sizeof(unsigned char) );

        char* frame_string = reinterpret_cast<char*>(in_data);

        UdpTransmitSocket transmitSocket = UdpTransmitSocket ( IpEndpointName( ADDRESS, PORT ) );

        char buffer[data_size];

        osc::OutboundPacketStream p( buffer, data_size );
        for (int y = 0 ; y < frame_height; ++y) {
            p << osc::BeginBundleImmediate
              << osc::BeginMessage( "/frame" );
            for ( int x = 0; x < frame_width; ++x) {
                p << (frame_string[x + y]);
            }
            p << osc::EndMessage << osc::EndBundle;

            transmitSocket.Send( p.Data(), p.Size() );
            p.Clear();
            // transmitSocket.Send( frame_string , sizeof(char) * 8 );
        }

        // std::cout << "Sent some Frame OSC of size " << data_size << std::endl;

    }
private:
    ;

};


#endif