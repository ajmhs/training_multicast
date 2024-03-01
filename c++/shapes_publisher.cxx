/*
* (c) Copyright, Real-Time Innovations, 2020.  All rights reserved.
* RTI grants Licensee a license to use, modify, compile, and create derivative
* works of the software solely for use with RTI Connext DDS. Licensee may
* redistribute copies of the software provided that all such copies are subject
* to this license. The software is provided "as is", with no warranty of any
* type, including any warranty for fitness for any purpose. RTI is under no
* obligation to maintain or support the software. RTI shall not be liable for
* any incidental or consequential damages arising out of the use or inability
* to use the software.
*/

#include <iostream>

#include <dds/pub/ddspub.hpp>
#include <rti/util/util.hpp>      // for sleep()
#include <rti/config/Logger.hpp>  // for logging
// alternatively, to include all the standard APIs:
//  <dds/dds.hpp>
// or to include both the standard APIs and extensions:
//  <rti/rti.hpp>
//
// For more information about the headers and namespaces, see:
//    https://community.rti.com/static/documentation/connext-dds/7.2.0/doc/api/connext_dds/api_cpp2/group__DDSNamespaceModule.html
// For information on how to use extensions, see:
//    https://community.rti.com/static/documentation/connext-dds/7.2.0/doc/api/connext_dds/api_cpp2/group__DDSCpp2Conventions.html

#include "application.hpp"  // for command line parsing and ctrl-c
#include "shapes.hpp"
#include <cmath>
#include <sstream>

using std::cout;
using std::endl;
using std::ostringstream;

class ShapesWriterListener : public dds::pub::NoOpDataWriterListener<ShapeTypeExtended> {
     virtual void on_publication_matched(dds::pub::DataWriter<ShapeTypeExtended>& writer, 
        const dds::core::status::PublicationMatchedStatus& status) {
        
        cout << "on_publication_matched. Locators:" << endl;

        // Let's see who we matched with
        dds::core::InstanceHandle hInst = status.last_subscription_handle();

        dds::topic::SubscriptionBuiltinTopicData match = matched_subscription_data(writer, hInst);		
        for (auto loc : match.extensions().multicast_locators()) {
            ostringstream ss;
            
            if (rti::core::LocatorKind::UDPv4 == loc.kind()) {
                auto addr = loc.address();
                dds::core::ByteSeq::const_iterator itor = addr.end() - 4;
                for (; itor != addr.end(); ++itor) {
                    ss << static_cast<int>(*itor);
                    if (itor +1 != addr.end())
                        ss << '.';
                }
                ss << ':' << loc.port();
            }
            cout << "\t" << ss.str() << endl;
        }
        
    }
};

void run_publisher_application(unsigned int domain_id, unsigned int sample_count)
{
    // DDS objects behave like shared pointers or value types
    // (see https://community.rti.com/best-practices/use-modern-c-types-correctly)

    // Start communicating in a domain, usually one participant per application
    dds::domain::DomainParticipant participant(domain_id/*, participant_qos*/);

    // Dump the participants initial peers list
    cout << "Participant initial peers:" << endl;
    for (auto peer : participant.qos().policy<rti::core::policy::Discovery>().initial_peers()) {
        cout << "\t" << peer << endl;
    }
    cout << "Participant multicast receive addresses:" << endl;
    for (auto addr : participant.qos().policy<rti::core::policy::Discovery>().multicast_receive_addresses()) {
        cout << "\t" << addr << endl;
    }

    // Create a Topic with a name and a datatype
    dds::topic::Topic< ::ShapeTypeExtended> topic(participant, "Square");

    // Create a Publisher
    dds::pub::Publisher publisher(participant);

    // Create a ShapesWriterListener
    auto listener = std::make_shared<ShapesWriterListener>();
    
    // Create a DataWriter with default QoS and set the listener
    dds::pub::DataWriter< ::ShapeTypeExtended> writer(publisher, topic);
    writer.set_listener(listener);

    ::ShapeTypeExtended data;

    const int left = 15, top = 15, right = 248, bottom = 278; // limits
    const int shape_size = 30;
    int x = left-shape_size, y = bottom - top / 2;
    const float AMPLITUDE = 100.0f;
    const float FREQUENCY = 0.0475f;

    data.color("ORANGE");
    data.shapesize(shape_size);
    data.fillKind(ShapeFillKind::SOLID_FILL);
    
    // Main loop, write data
    std::cout << "Writing an orange square sine pattern. Ctrl+c to exit" << std::endl;
        
    unsigned int samples_written = 0;
    for (; !application::shutdown_requested && samples_written < sample_count; ++samples_written) {

        if (++x > right)
          x = left-shape_size;

        y = (int)(bottom - top) / 2 + AMPLITUDE * std::sin(FREQUENCY * x);
        
        data.x(x);
        data.y(y);

        
        writer.write(data);

        //rti::util::sleep(dds::core::Duration(0, 30000000));
        rti::util::sleep(dds::core::Duration(1));

    }
}

int main(int argc, char *argv[])
{

    using namespace application;

    // Parse arguments and handle control-C
    auto arguments = parse_arguments(argc, argv);
    if (arguments.parse_result == ParseReturn::exit) {
        return EXIT_SUCCESS;
    } else if (arguments.parse_result == ParseReturn::failure) {
        return EXIT_FAILURE;
    }
    setup_signal_handlers();

    // Sets Connext verbosity to help debugging
    rti::config::Logger::instance().verbosity(arguments.verbosity);

    try {
        run_publisher_application(arguments.domain_id, arguments.sample_count);
    } catch (const std::exception& ex) {
        // This will catch DDS exceptions
        std::cerr << "Exception in run_publisher_application(): " << ex.what()
        << std::endl;
        return EXIT_FAILURE;
    }

    // Releases the memory used by the participant factory.  Optional at
    // application exit
    dds::domain::DomainParticipant::finalize_participant_factory();

    return EXIT_SUCCESS;
}
