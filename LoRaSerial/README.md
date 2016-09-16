This program emulates a serial modem with XON/XOFF flow control over a 
LoRa link (869.525MHz, 250kHz BW) with a useful throughput of approx 
4kbit/sec.

The two stations take turns to transmit, meaning protocols that expect a 
full duplex link should work with reduced performance.

This has been tested with PPP to enable IP over LoRa; which although 
slow is usable for applications such as SSH, text-only web browsing and 
messaging.
