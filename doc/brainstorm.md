# Ripple: System Design Brainstorm

I want to try experimenting with the approach that Elon Musk uses when designing his rockets. What are the fundamental
limits associated with this project? How can I achieve the best possible performance? What even defines "best performance"?

## Limits & Features of NRF24L01(+)
- SPI control interface max of 10MHz
- RF on-air max data rate 2 Mbps
- Auto-ack and retry mechanisms for reliability
- Dynamic payloads for efficient TX/RX length
- ACK messages may contain payloads for more efficient data packing on RF channel

## Performance Goals
- Data transfer reliability of 95% or greater.
- Minimum real-world packet rate of 500kB/s or greater.
- Less than 5ms latency from packet RX to ready for application consumption
- Less than 5ms latency from TX request to being on-air


## Software Needs
- Dedicated thread that Ripple can control for Network processing

