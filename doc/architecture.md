# Ripple System Architecture
I am attempting to try and mimic the OSI model as appropriate. For the purposes of embedded systems and
what I want to achieve with the driver, I believe the highest layer I want to implement is the Session
layer.

## Driver Layers
The system will be broken up into several layers, with each layer corresponding to a thread running on
the device. The threads will act as managers of the data flow within the layer and prevent the user
from having to interact with lower level details of the network stack operation.

### Network Stack: Session Layer
Responsibilities:
- Managing connections with multiple endpoints
- Resource allocation/deallocation
- Authorization and authentication
- Can send/receive bulk data through stream interfaces

Primary Data Type: Session

### Network Stack: Transport Layer
Responsibilities:
- Segmentation
- Flow control
- Error handling
- Transfer reliability
- Checksum
- Multiplexing

Provides Two Services:
- Connection oriented transmissions (TCP, etc)
- Connection-less transmissions (UDP, etc)

Primary Data Type: ???

###  Network Stack: Network Layer
Responsibilities:
- Logical Addressing  (IPv4, IPv6)
- Path Determination
- Routing
- Data forwarding

Primary Data Type: Packet

### Network Stack: Data Link Layer
Responsibilities:
- Physical addressing
- Controls data tx/rx on the physical medium
- Data queuing and scheduling

Primary Data Type: Frame

### Net Device Driver
The network device driver will handle intialization and control


## Testing
Each layer thread manager will use inheritance to allow dependency injection for testing purposes. While
most embedded systems don't handle this structure well due to the increased memory costs, it's deemed
important enough for this system due to the complex interactions between layers. The inheritance will
allow the drivers to be mocked and isolate each layer entirely for required testing.
