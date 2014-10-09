FSRtool
=======

This is the source code for an auto-calibrating force-sensitive-resistor (FSR) trigger.

Description
-----------

FSRs can (among other usecases) be used for building a Z-min endstop for 3D printers.
For this one would place at least 3 FSRs under the printbed of the printer.
When the printer's hotend now touches the build plate the FSRs will change their resistance
according to the weight the printer puts on the build plate - much like a cheap kitchen scale.

The only problem with this is: If you place something onto the sensors the endstops will trigger.
If you're just connection the sensors directly to an analog/digital input on your machine's
controller you'll now have to use a potentiometer to adjust the trigger level of the sensors.

Now the FSRtool jumps in: 
The FSRtool reads the resistance value from all FSRs and decides on each value if the endstop
was triggered or if it's just noise (e.g. vibrations). Also it's auto-calibrating which means that
it's constantly reading the sensor values and averaging them. If the sensors change resistance
above or below configurable thresholds the endstop trigger output will trigger. If the resistance
of the sensors doesn't change for some time it will take this resistance as the new base level.

For you this means that you're totally free to remove your build plate, re-add it to your machine
or switch it with a totally other one - the FSRtool will take care of recalibrating itself.

Hardware
--------

This should work with any Arduino out there. It was developed on an Arduino Pro mini.
Connect one of the pins of your FSRs to A0, A1 and A2 (each FSR one Arduino pin).
Use 10k pull-down resistors on these inputs.
Connect the other pin of your FSRs to the 5v supply (3.3v will also work if this is the supply
voltage of your Arduino).
Don't forget to connect VCC & GND to your Arduino.
Now connect the Arduino pin 12 to the Z-min endstops input on your machine control board.
That's it, you're ready to go!

Remember that all these pins can be configured in the source of the FSRtool.

Alternatives
------------

This is a lot like WingTangWong's AutoTuningFSRTrigger (https://github.com/WingTangWong/AutoTuningFSRTrigger),
the biggest difference would be that you need to reset his software to use a new base-level
while the FSRtool is automatically adjusting it's base-level as soon as the resistance hasn't changed
for some time.

Original idea by JohnSL: https://github.com/JohnSL/FSR_Endstop
