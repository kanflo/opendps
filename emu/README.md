# OpenDPS Emulator

For all your non hardware related needs, OpenDPS can now run as an emulator on your PC:

```
cd emul
make && ./dpsemul
OpenDPS Emulator
Comms thread listening on UDP port 5005
Event thread listening on UDP port 5006
```

You can interact with the emulator just as you would with en ESP8266 proxy:

```
% dpsctl -d 127.0.0.1 -F 
Selected OpenDPS supports the cv and cc functions.
 
% dpsctl -d 127.0.0.1 -f cv 
Changed function.
 
% dpsctl -d 127.0.0.1 -p v=3300 i=500
```

Through the port at 5006 you can interact with the emulator, albeit not very much at the moment. Connect and type ```draw<enter>```:

```
nc -u 127.0.0.1 5006
draw
```

and an inaccurate ascii version of the UI gets drawn on the emulator stdout:

```
[Evt] Received 4 bytes from 127.0.0.1:52425 [draw]
Drawing UI
0.00V
0.000A
0.0V
---
```