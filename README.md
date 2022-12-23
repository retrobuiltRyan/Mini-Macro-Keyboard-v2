# Mini-Macro-Keyboard-v2
 A macro keyboard to improve application workflow by hardcoding macros, hotkeys, and shortcuts in a tiny keyboard. There is a mode button + OLED screen breakout that allows the user to add multiple macro profiles. A focus on backwards compatibility utilizing cheaper components and versatility was key so many USB input devices can be made cheaply. You can populate the board with a mix of rotary encoders, Cherry keys, or tactile switches. This keyboard was a built for hot-key Retropie commands in Raspberry Pi arcades.This PCB can be populated with up to 5 Cherry Mx Keys and 1 rotary encoders (or omit the encoders for 6 keys). An additional 2 key breakout board can be used (with a TRS cable) to locate 2 cherry keys in discrete location (like a foot pedal). 
In action here: https://youtu.be/HfOvEncG98M?t=264

Parts List:

| Qty | Description | DigiKey P/N| Notes |
| -------- | --------|--------| ---------------- |
| 1 | Arduino Pro Micro|ebay is cheaper| |
| 6| Cherry Mx Switch |CH196-ND| Or any cherry compatible switch. RGB type are recommended |
| 6| CHERRY MX KEYCAP |1568-PRT-15306-ND |Clear keycap is recommended|
|1|Rotary Encoder w/ Switch |987-1398-ND|Solder encoder jumpers to enable signal paths |
|1|Knob for Rotary shaft, 6mm dia |n/a just get it from ebay|optional|
|1|DIP PCB Mount 5 Pins 3.5mm Socket Headphone Stereo Audio Jack |CP1-3524NG-ND|optional|
|1|Resistor 330 OHM 1206 |RMCF1206JT330RCT-ND|for Addressable LED Data-in line|
|7|SK6812 MINI-E Addressable RGB LED 5V 3228 SMD |[Aliexpress](https://www.aliexpress.us/item/2255800289371100.html?spm=a2g0s.12269583.0.0.572c1e9ceHD99V&gatewayAdapt=glo2usa4itemAdapt&_randl_shipto=US)|solder "LED PWR" jumper if using addressable LEDs|
|1|SWITCH TACTILE SPST-NO 0.05A 12V (6x6mm) |679-2443-ND||
|2|1x12 Header 0.1" pitch|S6100-ND||
|8|Diode *(OPTIONAL)*, small signal, SOD-123 |1N4148W-13FDICT-ND|Solder Diode bypass jumpers if not using diodes. Diodes are optional|
