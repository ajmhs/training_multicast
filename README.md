# Console application based on Shapes which demonstrates use of multicast 

Skeleton created via
 
`rtiddsgen -language C++11 -platform x64Linux4gcc7.3.0 -example x64Linux4gcc7.3.0 -create makefiles -create typefiles -d c++11 shapes.idl`

The intention here is to get the discovery data and the user data to go via multicast.

## Application changes

The publisher and subscriber applications were modified to provide addition information about the initial peers list, and a listener was added to the DataWriter and DataReader to print locator information on a Publication or Subscription match.

## QoS Changes

The changes to make the application work via multicast were added to the USER_QOS_PROFILES.xml file in three parts:

The communication was forced to use UDPv4 by adding 

```xml
 <transport_builtin>
    <mask>UDPv4</mask>
</transport_builtin>               
```
to the `domain_participant_qos` element.  


For the discovery to occur via multicast, the following snippet was added to the `domain_participant_qos` element:
```xml
 <discovery>                    
    <!-- define initial peers as a multicast net -->
    <initial_peers>
        <element>builtin.udpv4://239.255.0.1</element>                        
    </initial_peers>
    <multicast_receive_addresses>
        <element>239.255.0.1</element>
    </multicast_receive_addresses>
    <accept_unknown_peers>false</accept_unknown_peers>
</discovery>
```

This defines the initial peers to be at the `239.255.0.1` multicast address, and to also use that same address to receive data. Additionally unknown peers are not accepted.

And finally, in the definition of the DataReader (thus far the outbound address/network cannot be specified in Connext), the receiving address is defined, the port is omitted to allow Connext to automatically assign one.

```xml
<datareader_qos>
    <subscription_name>
        <name>shapesDataReader</name>
    </subscription_name>                        
    <multicast>
        <value>
            <element>
                <receive_address>239.255.0.2</receive_address>
            </element>
        </value>
    </multicast>
</datareader_qos>
```

I've deliberately assigned a different multicast address for the user traffic, as it makes it easier to differentiate when looking at a WireShark capture.

When running WireShark, it can be seen that discovery happens on the 239.255.0.1 multicast address, whilst the user data is sent on the 239.255.0.2 address. In this case, as both of the applications are running locally, the 
meta-traffic appears on the localhost address.


## Data(p) packet in wireshark
```
Frame 404: 864 bytes on wire (6912 bits), 864 bytes captured (6912 bits) on interface any, id 0
Linux cooked capture v1
Internet Protocol Version 4, Src: 192.168.40.1, Dst: 239.255.0.1
User Datagram Protocol, Src Port: 56965, Dst Port: 7400
Real-Time Publish-Subscribe Wire Protocol
    Magic: RTPS
    Protocol version: 2.5
    vendorId: 01.01 (Real-Time Innovations, Inc. - Connext DDS)
    guidPrefix: 0101c053718dea1913034c16
    Default port mapping: MULTICAST_METATRAFFIC, domainId=0
    submessageId: INFO_TS (0x09)
        Flags: 0x01, Endianness
        octetsToNextHeader: 8
        Timestamp: Mar  1, 2024 11:51:50.340614998 UTC
    submessageId: DATA (0x15)
        Flags: 0x05, Data present, Endianness
        octetsToNextHeader: 784
        0000 0000 0000 0000 = Extra flags: 0x0000
        Octets to inline QoS: 16
        readerEntityId: ENTITYID_UNKNOWN (0x00000000)
        writerEntityId: ENTITYID_BUILTIN_PARTICIPANT_WRITER (0x000100c2)
        writerSeqNumber: 1
        serializedData
```

## Data packet in wireshark

```
Frame 455: 160 bytes on wire (1280 bits), 160 bytes captured (1280 bits) on interface any, id 0
Linux cooked capture v1
Internet Protocol Version 4, Src: 192.168.40.1, Dst: 239.255.0.2
User Datagram Protocol, Src Port: 54269, Dst Port: 7401
Real-Time Publish-Subscribe Wire Protocol
    Magic: RTPS
    Protocol version: 2.5
    vendorId: 01.01 (Real-Time Innovations, Inc. - Connext DDS)
    guidPrefix: 0101ebb22ee23900cc4f9adb
    Default port mapping: MULTICAST_USERTRAFFIC, domainId=0
    submessageId: INFO_TS (0x09)
    submessageId: DATA (0x15)
        Flags: 0x07, Data present, Inline QoS, Endianness
        octetsToNextHeader: 80
        0000 0000 0000 0000 = Extra flags: 0x0000
        Octets to inline QoS: 16
        readerEntityId: ENTITYID_UNKNOWN (0x00000000)
        writerEntityId: 0x80000002 (Application-defined writer (with key): 0x800000)
        [Topic Information (from Discovery)]
            [typeName: ShapeTypeExtended]
            [topic: Square]
            [DCPSPublicationData In: 333]
        writerSeqNumber: 2
        inlineQos:
        serializedData (TypeId: 0x4328188006887bd3)(BaseId: 0x69fd54c6deb1ebc5)
```
