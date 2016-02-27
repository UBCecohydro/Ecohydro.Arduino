This is the UBC EcoHydro shield used in Costa Rica in 2015.

The version shown here is revised to remove some errors, it has not been sent to production yet.

RELEASE NOTES

Pending changes
    - GSM shield bypass pins are out by 1 (should be 2 & 3, not 3 & 4)
    - Add 2032 battery to BoM
    - Change package on xtal
    - Change R19 to 4k7
    - Change pads on Batt and Solar connectors
    - Thermal, regulator gets hot when 5V @ 150mA

Unreleased
    - R19 change to 0805
    - Transistor pinout was incorrect, fixed
    - Should be using mega pins 8-15, not pins 7-14 (Not all pins on the Mega and Mega 2560 support change interrupts, so only the following can be used for RX: 10, 11, 12, 13, 14, 15, 50, 51, 52, 53, A8 (62), A9 (63), A10 (64), A11 (65), A12 (66), A13 (67), A14 (68), A15 (69).)
    - DS1306 interrupt should be on pin 20 not pin 5
    - label fuses

Rev 1.0 Costa Rica 2015
    - Working
