# mpris-ctl 

Is a minimalistic cli tool for controlling audio players exposing a MPRIS DBus interface, targeted at keyboard based WMs.

Its only build/run dependency is on the [C dbus library](https://dbus.freedesktop.org/doc/api/html/index.html).

## Build

To build from source just clone the repository and run make. 
By default the binary is installed in **/usr/local/bin**, but you can provide your own DESTDIR and INSTALL_PREFIX.

````
$ git clone https://github.com/mariusor/mpris-ctl.git
$ cd mpris-ctl
$ make 
# make install
````

## Usage

An example of configuration for i3/sway:

````
bindsym XF86AudioPlay exec "mpris-ctl pp"
bindsym XF86AudioStop exec "mpris-ctl stop"
bindsym XF86AudioNext exec "mpris-ctl next"
bindsym XF86AudioPrev exec "mpris-ctl prev"
````

Supported format specifiers for `mpris-ctl info` command:

```
Format specifiers:
	%track_name	prints the track name
	%track_number	prints the track number
	%track_length	prints the track length (seconds)
	%artist_name	prints the artist name
	%album_name	prints the album name
	%album_artist	prints the album artist
	%play_status	prints the playback status
	%shuffle	prints the shuffle mode
	%volume		prints the volume
	%loop_status	prints the loop status
	%position	prints the song position (seconds)
	%bitrate	prints the track's bitrate
    %comment	prints the track's comment
```

Example: 

```
$ mpris-ctl info "%track_name"
Song 42
$ mpris-ctl info
Song 42 - Bloor - The Best of Bloor

```
