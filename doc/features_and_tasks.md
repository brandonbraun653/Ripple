- Broadcast device identifier packets on a particular address and include a unique node identifier
- Run the manager threads for each level of the stack using a class thread (ETL function binding, Chimera modification)
- More responsibly divide resources between the OSI layer
- Add utility methods for proper bit setting/clearing
- Have a single global handle type with opaque pointers to layer specific information. Pass this through the stack.
- Classes are centered threads that implement management for the OSI layers. They should NOT have a device interface.