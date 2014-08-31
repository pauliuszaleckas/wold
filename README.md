wold
====

Very simple Wake-on-Lan packet handler with custom action execution.

## Scratch your own itch
### My usage
I have a raspberry pi with raspbian installed. I manually installed XBMC and wish to be able to start XBMC directly (without ssh in and start the XBMC manually) using the Power button in the [Yatse remote control app] (https://play.google.com/store/apps/details?id=org.leetzone.android.yatsewidgetfree) on my Android phone. I googled and found this program, afer modifying it a bit, my wish came true. To achieve this yourself, just follow the Compiling/Installing and Usage instuctions that follow

### Original Usage (from the original author @pauliuszaleckas )
I started coding this small app to make XBMC start on my PC when "Power On" is pressed on Android remote control.
It is possible to wake my PC from suspend, but it boots to XFCE and second "Power On" turns on XBMC and disables VGA
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
Usage: wold -u user [-g group] [-p port] [-a action] [-f]
        -u username     run action as different user (required)
        -g group / gid  run action as different group / gid
        -p port         port to listen for WOL packet (default: 9)
        -f              stay in foreground
        -a action       application to run on WOL (default: /etc/wol.action)
```
To run XBMC (in raspbian), you can just put this line into */etc/rc.local*:

`DISPLAY=:0.0 /usr/local/bin/wold -u pi -g video -a /usr/bin/xbmc`

Restart you raspbian, then you will be able to start the XBMC using Yaste's power button.
