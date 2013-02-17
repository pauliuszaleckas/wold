wold
====

Very simple Wake-on-Lan packet handler with custom action execution.

## Scratch your own itch
I statrted coding this small app to make XBMC start on my PC when "Power On" is pressed on Android remote control.
It is posible to wake my PC from suspend, but it boots to XFCE and second "Power On" turns on XBMC and disables VGA
while enabling HDMI output.

## Compiling/Installing
To compile:

`make`

To install (usually you will need to be root)

`make install`

By default wold is installed to */usr/local/bin*.
If you want to install to more common place like */usr/bin*:

`make install PREFIX=/usr`

## Usage
```
Usage: wold -u user [-p port] [-a action] [-f]
	-u username	run action as different user (required)
	-p port		port to listen for WOL packet (default: 9)
	-f			stay in foreground
	-a action	application to run on WOL (default: /etc/wol.action)
```
