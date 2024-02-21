# Firmware for Status Indicator Display (SID)

This repository holds the most current firmware for CircuitSetup's magnificent [SID](https://circuitsetup.us/product/delorean-time-machine-status-indicator-display-sid/) kit. The SID, also known as "Field Containment System Display", is an important part of Doc Brown's Delorean Time Machine.

The parts are available [here](https://circuitsetup.us/product/delorean-time-machine-status-indicator-display-sid/).

![mysid](https://github.com/realA10001986/SID/assets/76924199/cdd8f609-1248-41f2-92cc-0489fe0397bf)

Features include
- various idle patterns
- [Time Travel](#time-travel) function, triggered by button, [Time Circuits Display](https://tcd.out-a-ti.me) or via [MQTT](#home-assistant--mqtt)
- [IR remote controlled](#ir-remote-control); can learn keys from third-party remote
- Spectrum Analyzer mode via built-in microphone
- Advanced network-accessible [Config Portal](#the-config-portal) for setup (http://sid.local, hostname configurable)
- [Wireless communication](#bttf-network-bttfn) with [Time Circuits Display](https://tcd.out-a-ti.me); used for synchonized time travels, GPS-speed adapted patterns, alarm, night mode, fake power and remote control through TCD keypad
- [Home Assistant](#home-assistant--mqtt) (MQTT 3.1.1) support
- [*Siddly*](#siddly) and [*Snake*](#snake) games
- [SD card](#sd-card) support

## Installation

If a previous version of the SID firmware is installed on your device, you can update easily using the pre-compiled binary. Enter the [Config Portal](#the-config-portal), click on "Update" and select the pre-compiled binary file provided in this repository ([install/sid-A10001986.ino.nodemcu-32s.bin](https://github.com/realA10001986/SID/blob/main/install/sid-A10001986.ino.nodemcu-32s.bin)).

If you are using a fresh ESP32 board, please see [sid-A10001986.ino](https://github.com/realA10001986/SID/blob/main/sid-A10001986/sid-A10001986.ino) for detailed build information.

 *Important: After a firmware update, a "wait" symbol (hourglass) might be shown for a short while after reboot. Do NOT unplug the device during this time.*

## Initial Configuration

The first step is to establish access to the SID's configuration web site ("Config Portal") in order to configure your SID:

- Power up the SID and wait until the startup sequence has completed.
- Connect your computer or handheld device to the WiFi network "SID-AP".
- Navigate your browser to http://sid.local or http://192.168.4.1 to enter the Config Portal.

#### Connecting to a WiFi network

As long as the device is unconfigured, it creates a WiFi network of its own named "SID-AP". This is called "Access point mode", or "AP-mode". 

It is ok to leave the SID in this mode, especially if it run stand-alone. In a typical home setup and/or if you also have a [Time Circuits Display](https://tcd.out-a-ti.me), however, you might want to connect the SID to a WiFi network (in case of using it together with a TCD: to the same WiFi network the TCD is connected to). If you have your SID, along with a Time Circuits Display, mounted in a car, you might want to connect the SID to the TCD's very own WiFi network "TCD-AP"; see [here](#car-setup).

In order to connect your SID to a WiFi network, click on "Configure WiFi". The bare minimum is to select an SSID (WiFi network name) and a WiFi password.

>Note that the SID requests an IP address via DHCP, unless you entered valid data in the fields for static IP addresses (IP, gateway, netmask, DNS). If the device is inaccessible as a result of incorrect static IPs, wait until the SID has completed its startup sequence, then type \*123456OK on the IR remote; static IP data will be deleted and the device will return to DHCP after a reboot.

After saving the WiFi network settings, the SID reboots and tries to connect to your configured WiFi network. If that fails, it will again start in access point mode.

After completing this step, your SID is basically ready for use; you can also continue configuring it to your personal preferences through the Config Portal.

## The Config Portal

The "Config Portal" is the SID's configuration web site. 

It can be accessed as follows:

#### If SID is in AP mode

- Connect your computer or handheld device to the WiFi network "SID-AP".
- Navigate your browser to http://sid.local or http://192.168.4.1 to enter the Config Portal.

#### If SID is connected to WiFi network

- Connect your hand-held/computer to the same WiFi network to which the SID is connected, and
- navigate your browser to http://sid.local

  Accessing the Config Portal through this address requires the operating system of your hand-held/computer to support Bonjour/mDNS: Windows 10 version TH2     (1511) [other sources say 1703] and later, Android 13 and later; MacOS and iOS since the dawn of time.

  If connecting to http://sid.local fails due to a name resultion error, you need to find out the SID's IP address: Type *90 followed by OK on the remote control; the IP address will be shown on the display. Then, on your handheld or computer, navigate to http://a.b.c.d (a.b.c.d being the IP address as shown on the SID's display) in order to enter the Config Portal.

In the main menu, click on "Setup" to configure your SID. 

| ![The Config Portal](https://github.com/realA10001986/SID/assets/76924199/8c60dc7a-9d3c-4b65-be3a-411640874c48) |
|:--:| 
| *The Config Portal's Setup page* |

A full reference of the Config Portal is [here](#appendix-a-the-config-portal).

## Basic Operation

When the SID is idle, it shows an idle pattern. There are alternative idle patterns to choose from, selected by *10OK through *14OK on the remote, or via MQTT. If an SD card is present, the setting will be persistent accross reboots.

If the option **_Adhere strictly to movie patterns_** is set (which is the default), the idle patterns #0 through #3 will only use patterns extracted from the movies (plus some interpolations); the same goes for when [GPS speed](#bttf-network-bttfn) is used. If this option is unset, random variations are shown, which is less boring, but also less accurate.

For ways to trigger a time travel, see [here](#time-travel).

The main control device is the supplied IR remote control. If a TCD is connected through [BTTF-Network](#bttf-network-bttfn), the SID can also be controlled through the TCD's keypad.

### IR remote control

Your SID has an IR remote control included. This remote works out-of-the-box and needs no setup. 

| ![Supplied IR remote control](https://github.com/realA10001986/SID/assets/76924199/2a3bfd40-2a44-4cc0-8209-13468115ae17) |
|:--:| 
| *The default IR remote control* |

Each time you press a (recognized) key on the remote, an IR feedback LED will briefly light up. This LED is located at the bottom of the board.

### IR learning

Your SID can learn the codes of another IR remote control. Most remotes with a carrier signal of 38kHz (which most IR remotes use) will work. However, some remote controls, expecially ones for TVs, send keys repeatedly and/or send different codes alternately. If you had the SID learn a remote and the keys are not (always) recognized afterwards or appear to the pressed repeatedly while held, that remote is of that type and cannot be used.

First, go to the Config Portal, uncheck **_TCD connected by wire_** on the Setup page and save. The SID reboots. Afterwards, to start the learning process, hold the [Time Travel](#time-travel) button for a few seconds, until the displays shows "GO" followed by "0". Then press "0" on your remote, which the SID will visually acknowledge by displaying the next key to press. Then press "1", wait for the acknowledgement, and so on. Enter your keys in the following order:

```0 - 1 - 2 - 3 - 4 - 5 - 6 - 7 - 8 - 9 - * - # - Arrow up - Arrow down - Arrow left - Arrow right - OK``` 

If your remote control lacks the \* (starts command sequence) and \# (aborts command sequence) keys, you can use any other key, of course. \* could be eg. "menu" or "setup", \# could be "exit" or "return".

If no key is pressed for 10 seconds, the learning process aborts, as does briefly pressing the Time Travel button. In thoses cases, the keys already learned are forgotten and nothing is saved.

To make the SID forget a learned IR remote control, type *654321 followed by OK.

### Locking IR control

You can have your SID ignore IR commands from any IR remote control (be it the default supplied one, be it one you had the SID learn) by entering *71 followed by OK. After this sequence the SID will ignore all IR commands until *71OK is entered again. The purpose of this function is to enable you to use the same IR control for your SID and other props (such as Flux Capacitor).

Note that the status of the IR lock is saved 10 seconds after its last change, and is persistent accross reboots.

In order to only disable the supplied IR remote control, check the option **_Disable supplied IR remote control_** in the [Config Portal](#-disable-supplied-ir-remote-control). In that case, any learned remote will still work.

### Remote control reference

<table>
    <tr>
     <td align="center" colspan="3">Single key actions</td>
    </tr>
    <tr>
     <td align="center">1<br>Games: Quit</td>
     <td align="center">2<br>-</td>
     <td align="center">3<br>Games: New game</a></td>
    </tr>
    <tr>
     <td align="center">4<br>-</td>
     <td align="center">5<br>-</td>
     <td align="center">6<br>-</td>
    </tr>
    <tr>
     <td align="center">7<br>-</td>
     <td align="center">8<br>-</td>
     <td align="center">9<br>Games: Pause</td>
    </tr>
    <tr>
     <td align="center">*<br>Start command sequence</td>
     <td align="center">0<br><a href="#time-travel">Time Travel</a><br>Siddly: Fall down</td>
     <td align="center">#<br>Abort command sequence</td>
    </tr>
    <tr>
     <td align="center"></td>
     <td align="center">&#8593;<br>Increase Brightness<br>Siddly: Rotate<br>Snake: Up</td>
     <td align="center"></td>
    </tr>
    <tr>
     <td align="center">&#8592;<br>Games: Left</td>
     <td align="center">OK<br>Execute command</td>
     <td align="center">&#8594;<br>Games: Right</td>
    </tr>
    <tr>
     <td align="center"></td>
     <td align="center">&#8595;<br>Decrease Brightness<br>Games: Down</td>
     <td align="center"></td>
    </tr>
</table>

<table>
    <tr>
     <td align="center" colspan="3">Special sequences<br>(&#9166; = OK key)</td>
    </tr>
   <tr><td>Function</td><td>Code on IR</td><td>Code on TCD</td></tr>
    <tr>
     <td align="left">Default idle pattern</td>
     <td align="left">*10&#9166;</td><td>6010</td>
    </tr>
    <tr>
     <td align="left">Idle pattern 1</td>
     <td align="left">*11&#9166;</td><td>6011</td>
    </tr>
    <tr>
     <td align="left">Idle pattern 2</td>
     <td align="left">*12&#9166;</td><td>6012</td>
    </tr>
    <tr>
     <td align="left">Idle pattern 3</td>
     <td align="left">*13&#9166;</td><td>6013</td>
    </tr>
     <tr>
     <td align="left">Idle pattern 4</td>
     <td align="left">*14&#9166;</td><td>6014</td>
    </tr>
    <tr>
     <td align="left">Switch to idle mode</td>
     <td align="left">*20&#9166;</td><td>6020</td>
    </tr>
    <tr>
     <td align="left">Start Spectrum Analyzer</td>
     <td align="left">*21&#9166;</td><td>6021</td>
    </tr>
    <tr>
     <td align="left">Start Siddly game</td>
     <td align="left">*22&#9166;</td><td>6022</td>
    </tr>
    <tr>
     <td align="left">Start Snake game</td>
     <td align="left">*23&#9166;</td><td>6023</td>
    </tr>
    <tr>
     <td align="left">Enable/disable "<a href="#-adhere-strictly-to-movie-patterns">strictly movie patterns</a>"</td>
     <td align="left">*50&#9166;</td><td>6050</td>
    </tr>
   <tr>
     <td align="left">Enable/disable peaks in Spectrum Analyzer</td>
     <td align="left">*51&#9166;</td><td>6051</td>
    </tr>
    <tr>
     <td align="left"><a href="#locking-ir-control">Disable/Enable</a> IR remote commands</td>
     <td align="left">*71&#9166;</td><td>6071</td>
    </tr>
    <tr>
     <td align="left">Display current IP address</td>
     <td align="left">*90&#9166;</td><td>6090</td>
    </tr>
   <tr>
     <td align="left">Set brightness level (00-15)</td>
     <td align="left">*400&#9166; - *415&#9166;</td><td>6400-6415</td>
    </tr>
    <tr>
     <td align="left">Reboot the device</td>
     <td align="left">*64738&#9166;</td><td>6064738</td>
    </tr>
    <tr>
     <td align="left">Delete static IP address<br>and WiFi-AP password</td>
     <td align="left">*123456&#9166;</td><td>6123456</td>
    </tr>
    <tr>
     <td align="left">Delete learned IR remote control</td>
     <td align="left">*654321&#9166;</td><td>6654321</td>
    </tr>
</table>

[Here](https://github.com/realA10001986/SID/blob/main/CheatSheet.pdf) is a cheat sheet for printing or screen-use. (Note that MacOS' preview application has a bug that scrambles the links in the document. Acrobat Reader does it correctly.)

## Time travel

To travel through time, type "0" on the remote control. The SID will play its time travel sequence.

You can also connect a physical button to your SID; the button must shorten "TT" and "3.3V" on the "Time Travel" connector. Pressing this button briefly will trigger a time travel.

Other ways of triggering a time travel are available if a [Time Circuits Display](#connecting-a-time-circuits-display) is connected.

## Spectrum Analyzer

The spectrum analyzer (or rather: frequency-separated vu meter) works through a built-in microphone. This microphone is located behind the right hand side center hole of the enclosure.

Sticky peaks are optional, they can be switched on/off in the Config Portal and by typing *51 followed by OK on the remote.

## Games

### Siddly

Siddly is a simple game where puzzle pieces of various shapes fall down from the top. You can slide them left and right, as well as rotate them while they are falling. When the piece lands at the bottom, a new piece will appear at the top and start falling down. If a line at the bottom is completely filled with fallen pieces or parts thereof, that line will be cleared, and everything piled on top of that line will move down. The target is to keep the pile at the bottom as low as possible; the game ends when the pile is as high as the screen and no new piece has room to appear. I think you get the idea. Note that the red LEDs at the top are not part of the playfield (but show a level-progress bar instead), the field only covers the yellow and green LEDs, and that simularities of Siddly with computer games, especially older ones, exist only in your imagination.

### Snake

Snakes like apples (at least so I have heard). You control a snake that feels a profound urge to eat apples. After each eaten apple, the snake grows, and a new apple appears. Unfortunately, snakes don't like to hit their heads, so you need to watch out that the snake's head doesn't collide with its body.

## SD card

Preface note on SD cards: For unknown reasons, some SD cards simply do not work with this device. For instance, I had no luck with Sandisk Ultra 32GB and  "Intenso" cards. If your SD card is not recognized, check if it is formatted in FAT32 format (not exFAT!). Also, the size must not exceed 32GB (as larger cards cannot be formatted with FAT32). Transcend SDHC cards and those work fine in my experience.

The SD card is used for saving [secondary settings](#-save-secondary-settings-on-sd), in order to avoid [Flash Wear](#flash-wear) on the SID's ESP32. The chosen idle pattern (*1x), along with the ["strictly movie patterns"](#-adhere-strictly-to-movie-patterns) setting, is only stored on SD, so for your selection to be persistent accross reboots, an SD card is required. 

Note that the SD card must be inserted before powering up the device. It is not recognized if inserted while the SID is running. Furthermore, do not remove the SD card while the device is powered.

## Connecting a Time Circuits Display

### Connecting a TCD by wire

Note that a wired connection only allows for synchronized time travel sequences, no other communication takes place. Therefore I strongly recommend a wireless BTTFN connection, see immediately below.

For a connection by wire, connect GND and GPIO on the SID's "Time Travel" connector to the TCD like in the table below:

<table>
    <tr>
     <td align="center">SID</td>
     <td align="center">TCD with control board 1.2</td>
     <td align="center">TCD with control board 1.3</td>
    </tr>
   <tr>
     <td align="center">GND of 3-pin connector</td>
     <td align="center">GND of "IO14" connector</td>
     <td align="center">GND on "Time Travel" connector</td>
    </tr>
    <tr>
     <td align="center">TT of 3-pin connector</td>
     <td align="center">IO14 of "IO14" connector</td>
     <td align="center">TT OUT on "Time Travel" connector</td>
    </tr>
</table>

Next, head to the Config Portal and set the ooption **_TCD connected by wire_**. On the TCD, the option "Control props connected by wire" must be set.

You can connect both the TCD and a button to the TT connector, which might be handy in case you want to have the FC learn an IR remote control without fiddling with cables. But the button should not be pressed when the option **_TCD connected by wire_** is set, as it might yield unwanted results. Also, note that the button connects to IO13 and 3_3V (not GND!).

### BTTF-Network ("BTTFN")

The TCD can communicate with the SID wirelessly, via the built-in "**B**asic-**T**elematics-**T**ransmission-**F**ramework" over WiFi. It can send out information about a time travel and an alarm, and the SID queries the TCD for speed and some other data. Furthermore, the TCD's keypad can be used to remote-control the SID.

| [![Watch the video](https://img.youtube.com/vi/u9oTVXUIOXA/0.jpg)](https://youtu.be/u9oTVXUIOXA) |
|:--:|
| Click to watch the video |

Note that the TCD's firmware must be up to date for BTTFN. You can use [this](http://tcd.out-a-ti.me) one or CircuitSetup's release 2.9 or later.

![BTTFN connection](https://github.com/realA10001986/SID/assets/76924199/404e4dd5-ac31-4ca4-b6a1-c8084d3a82e0)

In order to connect your SID to the TCD using BTTFN, just enter the TCD's IP address or hostname in the **_IP address or hostname of TCD_** field in the SID's Config Portal. On the TCD, no special configuration is required. Note that you need TCD firmware 2.9.1 or later for using a hostname; previous versions only work with an IP address.

Afterwards, the SID and the TCD can communicate wirelessly and 
- play time travel sequences in sync,
- both play an alarm-sequence when the TCD's alarm occurs,
- the SID can be remote controlled through the TCD's keypad (command codes 6xxx),
- the SID queries the TCD for GPS speed if desired to adapt its idle pattern to GPS speed,
- the SID queries the TCD for fake power and night mode, in order to react accordingly if so configured,
- pressing "0" on the IR remote control or the SID's Time Travel button can trigger a synchronized Time Travel on all BTTFN-connected devices, just like if that Time Travel was triggered through the TCD.

You can use BTTF-Network and MQTT at the same time, see immediately below.

## Home Assistant / MQTT

The SID supports the MQTT protocol version 3.1.1 for the following features:

### Control the SID via MQTT

The SID can - to a some extent - be controlled through messages sent to topic **bttf/sid/cmd**. Support commands are
- TIMETRAVEL: Start a [time travel](#time-travel)
- IDLE: Switch to idle mode
- SA: Start spectrum analyzer
- IDLE_0, IDLE_1, IDLE_2, IDLE_3, IDLE_4: Select idle pattern

### Receive commands from Time Circuits Display

If both TCD and SID are connected to the same broker, and the option **_Send event notifications_** is checked on the TCD's side, the SID will receive information on time travel and alarm and play their sequences in sync with the TCD. Unlike BTTFN, however, no other communication takes place.

![MQTT connection](https://github.com/realA10001986/SID/assets/76924199/4a7f3c63-91cf-4206-af36-ab39c28dfb3e)

MQTT and BTTFN can co-exist. However, the TCD only sends out time travel and alarm notifications through either MQTT or BTTFN, never both. If you have other MQTT-aware devices listening to the TCD's public topic (bttf/tcd/pub) in order to react to time travel or alarm messages, use MQTT (ie check **_Send event notifications_**). If only BTTFN-aware devices are to be used, uncheck this option to use BTTFN as it has less latency.

### Setup

In order to connect to a MQTT network, a "broker" (such as [mosquitto](https://mosquitto.org/), [EMQ X](https://www.emqx.io/), [Cassandana](https://github.com/mtsoleimani/cassandana), [RabbitMQ](https://www.rabbitmq.com/), [Ejjaberd](https://www.ejabberd.im/), [HiveMQ](https://www.hivemq.com/) to name a few) must be present in your network, and its address needs to be configured in the Config Portal. The broker can be specified either by domain or IP (IP preferred, spares us a DNS call). The default port is 1883. If a different port is to be used, append a ":" followed by the port number to the domain/IP, such as "192.168.1.5:1884". 

If your broker does not allow anonymous logins, a username and password can be specified.

Limitations: MQTT Protocol version 3.1.1; TLS/SSL not supported; ".local" domains (MDNS) not supported; server/broker must respond to PING (ICMP) echo requests. For proper operation with low latency, it is recommended that the broker is on your local network. 

## Car setup

If your SID, along with a [Time Circuits Display](https://tcd.out-a-ti.me), is mounted in a car, the following network configuration is recommended:

#### TCD

- Run your TCD in [*car mode*](https://tcd.out-a-ti.me/#car-mode);
- disable WiFi power-saving on the TCD by setting **_WiFi power save timer for AP-mode_** to 0 (zero).

#### SID

Enter the Config Portal on the SID (as described above), click on *Setup* and
  - enter *192.168.4.1* into the field **_IP address or hostname of TCD_**
  - check the option **_Follow TCD fake power_** if you have a fake power switch for the TCD (like eg a TFC switch)
  - click on *Save*.

After the SID has restarted, re-enter the SID's Config Portal (while the TCD is powered and in *car mode*) and
  - click on *Configure WiFi*,
  - select the TCD's access point name in the list at the top or enter *TCD-AP* into the *SSID* field; if you password-protected your TCD's AP, enter this password in the *password* field. Leave all other fields empty,
  - click on *Save*.

Using this setup enables the SID to receive notifications about time travel and alarm wirelessly, and to query the TCD for data. Also, the TCD keypad can be used to remote-control the SID.

In order to access the SID's Config Portal in your car, connect your hand held or computer to the TCD's WiFi access point ("TCD-AP"), and direct your browser to http://sid.local ; if that does not work, go to the TCD's keypad menu, press ENTER until "BTTFN CLIENTS" is shown, hold ENTER, and look for the SID's IP address there; then direct your browser to that IP by using the URL http://a.b.c.d (a-d being the IP address displayed on the TCD display).

## Flash Wear

Flash memory has a somewhat limited life-time. It can be written to only between 10.000 and 100.000 times before becoming unreliable. The firmware writes to the internal flash memory when saving settings and other data. Every time you change settings, data is written to flash memory.

In order to reduce the number of write operations and thereby prolong the life of your SID, it is recommended to use a good-quality SD card and to check **_["Save secondary settings on SD"](#-save-secondary-settings-on-sd)_** in the Config Portal; some settings as well as learned IR codes are then stored on the SD card (which also suffers from wear but is easy to replace). See [here](#-save-secondary-settings-on-sd) for more information.

## Appendix A: The Config Portal

### Main page

##### &#9654; Configure WiFi

Clicking this leads to the WiFi configuration page. On that page, you can connect your SID to your WiFi network by selecting/entering the SSID (WiFi network name) as well as a password (WPA2). By default, the SID requests an IP address via DHCP. However, you can also configure a static IP for the SID by entering the IP, netmask, gateway and DNS server. All four fields must be filled for a valid static IP configuration. If you want to stick to DHCP, leave those four fields empty.

Note that this page has nothing to do with Access Point mode; it is strictly for connecting your SID to an existing WiFi network as a client.

##### &#9654; Setup

This leads to the [Setup page](#setup-page).

##### &#9654; Update

This leads to the firmware update page. You can select a locally stored firmware image file to upload (such as the ones published here in the install/ folder).

##### &#9654; Erase WiFi Config

Clicking this (and saying "yes" in the confirmation dialog) erases the WiFi configuration (WiFi network and password) and reboots the device; it will restart in "access point" mode. See [here](#short-summary-of-first-steps).

---

### Setup page

#### Basic settings

##### &#9654; Screen saver timer

Enter the number of minutes until the Screen Saver should become active when the SID is idle.

The Screen Saver, when active, disables all LEDs, until 
- a key on the IR remote control is pressed; if IR is [locked](#locking-ir-control), only the # key deactivates the Screen Saver;
- the time travel button is briefly pressed (the first press when the screen saver is active will not trigger a time travel),
- on a connected TCD, a destination date is entered (only if TCD is wirelessly connected) or a time travel event is triggered (also when wired).

#### Hardware configuration settings

##### &#9654; Disable supplied IR remote control

Check this to disable the supplied remote control; the SID will only accept commands from a learned IR remote (if applicable). 

Note that this only disables the supplied remote, unlike [IR locking](#locking-ir-control), where IR commands from any known remote are ignored.

#### Network settings

##### &#9654; Hostname

The device's hostname in the WiFi network. Defaults to 'sid'. This also is the domain name at which the Config Portal is accessible from a browser in the same local network. The URL of the Config Portal then is http://<i>hostname</i>.local (the default is http://sid.local)

If you have more than one SID in your local network, please give them unique hostnames.

##### &#9654; AP Mode: Network name appendix

By default, if the SID creates a WiFi network of its own ("AP-mode"), this network is named "SID-AP". In case you have multiple SIDs in your vicinity, you can have a string appended to create a unique network name. If you, for instance, enter "-ABC" here, the WiFi network name will be "SID-AP-ABC". Characters A-Z, a-z, 0-9 and - are allowed.

##### &#9654; AP Mode: WiFi password

By default, and if this field is empty, the SID's own WiFi network ("AP-mode") will be unprotected. If you want to protect your SID access point, enter your password here. It needs to be 8 characters in length and only characters A-Z, a-z, 0-9 and - are allowed.

If you forget this password and are thereby locked out of your SID, enter *123456 followed by OK on the IR remote control; this deletes the WiFi password. Then power-down and power-up your SID and the access point will start unprotected.

##### &#9654; WiFi connection attempts

Number of times the firmware tries to reconnect to a WiFi network, before falling back to AP-mode. See [here](#short-summary-of-first-steps)

##### &#9654; WiFi connection timeout

Number of seconds before a timeout occurs when connecting to a WiFi network. When a timeout happens, another attempt is made (see immediately above), and if all attempts fail, the device falls back to AP-mode. See [here](#short-summary-of-first-steps)

#### Settings for prop communication/synchronization

##### &#9654; TCD connected by wire

Check this if you have a Time Circuits Display connected by wire. Note that a wired connection only allows for synchronized time travel sequences, no other communication takes place.

While you can connect both a button and the TCD to the "time travel" connector on the SID, the button should not be pressed when this option is set, as it might yield unwanted effects.

Also note that the process of [learning keys from an IR remote control](#ir-remote-control) requires this option to be unchecked. After learning keys is done, you can, of course, check this option again.

Do NOT check this option if your TCD is connected wirelessly (BTTFN, MQTT).

##### &#9654; TCD signals Time Travel without 5s lead

Usually, the TCD signals a time travel with a 5 seconds lead, in order to give a prop a chance to play an acceleration sequence before the actual time travel takes place. Since this 5 second lead is unique to CircuitSetup props, and people sometimes want to connect third party props to the TCD, the TCD has the option of skipping this 5 seconds lead. If that is the case, and your SID is connected by wire, you need to set this option.

If your SID is connected wirelessly, this option has no effect.

##### &#9654; IP address or hostname of TCD

If you want to have your SID to communicate with a Time Circuits Display wirelessly ("BTTF-Network"), enter the IP address of the TCD here. If your TCD is running firmware version 2.9.1 or later, you can also enter the TCD's hostname here instead (eg. 'timecircuits').

If you connect your SID to the TCD's access point ("TCD-AP"), the TCD's IP address is 192.168.4.1.

##### &#9654; Adapt to GPS speed

If this option is checked and your TCD is equipped with a GPS sensor or a rotary encoder, the SID will adapt its display pattern to current GPS speed or the reading of the encoder, respectively.

##### &#9654; Follow TCD night-mode

If this option is checked, and your TCD goes into night mode, the SID will activate the Screen Saver with a very short timeout. 

##### &#9654; Follow TCD fake power

If this option is checked, and your TCD is equipped with a fake power switch, the SID will also fake-power up/down. If fake power is off, no LED is active and the SID will ignore all input from buttons, knobs and the IR control.

##### &#9654; '0' and button trigger BTTFN-wide TT

If the SID is connected to a TCD through BTTFN, this option allows to trigger a synchronized time travel on all BTTFN-connected devices when pressing "0" on the IR remote control or pressing the Time Travel button, just as if the Time Travel was triggered by the TCD. If this option is unchecked, pressing "0" or the Time Travel button only triggers a Time Travel sequence on the SID.

##### &#9654; Show clock when Screen Saver is active

If this option is checked, the SID will show current local time - as queried from the TCD - when the Screen Saver is active.

#### Visual options

##### &#9654; Adhere strictly to movie patterns

If this option is checked, in idle modes 0-3 as well as when using GPS speed, only patterns which were extracted from the movies (plus some interpolations) are shown. If this option is unchecked, random variations will be shown, which is less accurate, but also less monotonous. Purists will want this option to be set, which is also the default. This option can also be changed by typing *50 followed by OK on the IR remote control.

Note that this option setting, along with the current idle pattern, is only saved if there is an SD card present. Without an SD card, this setting is always reset to "checked" upon power-up.

##### &#9654; Skip time tunnel animation

When set, the time travel sequence will not be animated (no flicker, no "moving bar"). Purists will want this option to be set; the default is unset.

##### &#9654; Show peaks in Spectrum Analyzer

This selects the boot-up setting for showing or not showing the peaks in the Spectrum Analyzer. Can be changed anytime by typing *51 followed by OK on the IR remote control.

#### Home Assistant / MQTT settings

##### &#9654; Use Home Assistant (MQTT 3.1.1)

If checked, the SID will connect to the broker (if configured) and send and receive messages via [MQTT](#home-assistant--mqtt)

##### &#9654; Broker IP[:port] or domain[:port]

The broker server address. Can be a domain (eg. "myhome.me") or an IP address (eg "192.168.1.5"). The default port is 1883. If different port is to be used, it can be specified after the domain/IP and a colon ":", for example: "192.168.1.5:1884". Specifiying the IP address is preferred over a domain since the DNS call adds to the network overhead. Note that ".local" (MDNS) domains are not supported.

##### &#9654; User[:Password]

The username (and optionally the password) to be used when connecting to the broker. Can be left empty if the broker accepts anonymous logins.

#### Other settings

##### &#9654; Save secondary settings on SD

If this is checked, secondary settings (brightness, IR lock status, learned IR keys) are stored on the SD card (if one is present). This helps to minimize write operations to the internal flash memory and to prolong the lifetime of your SID. See [Flash Wear](#flash-wear).

Apart from Flash Wear, there is another reason for using an SD card for settings: Writing data to internal flash memory can cause delays of up to 1.5 seconds, which interrupt sequences and have other undesired effects. The SID needs to save data from time to time, so in order for a smooth experience without unexpected and unwanted delays, please use an SD card and check this option.

It is safe to have this option checked even with no SD card present.

If you want copy settings from one SD card to another, do as follows:
- With the old SD card still in the slot, enter the Config Portal, turn off _Save secondary settings on SD_, and click "SAVE".
- After the SID has rebooted, power it down, and swap the SD card for your new one.
- Power-up the SID, enter the Config Portal, re-enable _Save secondary settings on SD_, and click "SAVE".

This procedure ensures that all your settings are copied from the old to the new SD card.
