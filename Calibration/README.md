This is a work-in-progress calibration tool for the low-cost HopeRF LoRa 
modules, to correct the often significant crystal offsets.

It works by using the RSSI function of the chipset in OOK mode to find a 
reference peak, and computing the offset between found frequency 
and expected frequency. 

A suitable reference peak must first be located using a SDR dongle or 
similar device, looking for some external transmitter producing a clean, 
continuous signal at a frequency within the band of the module you are 
calibrating. At the moment the frequency to search around is 
hard-coded.

In theory this tool should allow use at lower bandwidths/higher 
spreading factors without needing a TCXO.
