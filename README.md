BoilerMakeBadge_I
=================

# BoilerMake Badge #
Ha! You found it! We're stoked for these one of a kind BoilerMake Badges, and can't wait to see what all the hackers here at BoilerMake end up doing with them.

We put a lot of work into these, so we kinda have to brag a little... here's some quick facts you might find interesting.

- The badge is an **Arduino Dev Kit**. You can use the super easy to use Arduino software to program them from a PC, Mac, or Linux based machine.
- These badges were **designed entirely by Purdue students** [Thomas Kilbride](https://github.com/obnauticus) and [Lior Ben-Yehoshua](https://github.com/lior9999). [Scott Opell](https://github.com/scottopell) and [Viraj Sina](https://github.com/vsinha) both contributed significantly to the default code that was running on your badges when you first walked in here.
- We **maxed out** the capacity of the world's **3rd largest PCB manufacturer for two days** to make these boards. We're talking Foxconn level stuff here, guys. - We've **open sourced the firmware source code**, so feel free to hack/design/build away to your heart's content.

Ok, now onto business, we've come up with a little documentation and tutorial to get you hacking on our badge as soon as possible!

## Things You Need ##
1. Windows, Mac, or Linux PC (sorry no Solaris support...)
2. Micro USB cable
3. [Arduino IDE](http://arduino.cc/en/Main/Software)

## Connect to the Board ##
Each BoilerMake badge is equipped with a small nRF24L01 radio which allows it to send and receive messages from other badges. Here's how to do that:

1. After installing the Arduino IDE, plug in the board to your computer via micro USB cable.
2. All lights around the board should light up. If this not's the case, unplug and plug the board back in again.
3. Download this GitHub repo [here](https://github.com/maniacbug/RF24) as a .zip file.
4. Open the Arduino IDE and go to sketch -> import library -> add library -> find the rf24-master.zip and select it.
5. Copy and Paste the code found [here](https://raw.githubusercontent.com/obnauticus/BoilerMakeBadge_I/master/boilermakeBoard.cpp) into your new Arduino sketch.
6. Go to Tools -> Board and select Arduino Leonardo.
7. Hold the button on the badge down
9. Press the upload button (the arrow) in the Arduino IDE. When the IDE says "Uploading..." release this button.
9. Press Ctrl + Shift + m to open the Arduino console.
10. Change "No ending" to "Carriage Return" at the bottom of the console.
11. Your board's ID will appear at the top, you might want to take note of this as other boards will be able to communicate to you using this ID.
12. Type "help" for a list of commands.
13. Type "send <board id> -l <1, 2, 3, or 4>" to send a pattern to another board.
14. Hack!

## Customizing The Code ##
If you want to write your own firmware for the device, feel free. You can get and edit the default .cpp file that's been loaded on there here.
