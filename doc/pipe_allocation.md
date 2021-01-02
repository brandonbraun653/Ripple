# Some Thoughts
There are a total of 6 pipes/ports available on the NRF24L01 that need to be put to good use. I've already
tried the tree strategy of the most popular software for this device and found it to be pretty limiting.
I think it will be much easier to have the device configured like most modern networks with an IP address
and a MAC address. In this case, the device will have six MAC addresses, one for each pipe. The tradeoff
here is that more work will have to be done in software to handle the network configuration and data routing.

# Pipe Allocation
Pipe 0: Communication with PRX
  This will be used in conjunction with the TX address to listen for ACK events on the
  RX device. It seems to make sense that the hardware designers intended to have this
  pipe dedicated to this purpose. The remaining five pipes all share the same base address
  as pipe 1.

  When there is no on going TX in progress, the pipe will switch to listen to a global
  address that can be used for device discovery, multicasting, etc. This is important
  for lots of network level processes.

Pipe 1: Device Command & Control
  This pipe will define the root device address. All remaining pipes can only customize
  the first byte of their address and must inherit all other address bytes of this pipe.
  Packets used to interact with the device driver (change/query/command) will flow here.

Pipe 2: Network Services
  Pipe used for things like date & time information, maintenance messages, network statistics,
  etc. Essentially housekeeping data.

Pipe 3: Port Forwarding
  This pipe will be used to forward data onto another node in the network. Incoming data
  will make it up to the network layer and then be routed back down onto the next node.

Pipe 4: Application Data Port 1
  This pipe is for data ultimately destined for an application running on the host device.

Pipe 5: Application Data Port 2
  Secondary pipe for application data, to help increase throughput.
