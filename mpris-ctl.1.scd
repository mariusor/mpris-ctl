MPRIS-CTL(1)

# NAME

*mpris-ctl* - The MPRIS control utility

# SYNOPSIS

mpris-ctl [COMMAND] [format string]

# DESCRIPTION

*mpris-ctl* is a control utility for any media-player that conform to the *MPRIS D-Bus Interface Specification*[1]

# COMMANDS

*help*
	This help message

*play*
	Begin playing

*pp*
	Toggle play or pause

*pause*
	Pause the player

*stop*
	Stop the player

*next*
	Change to the next track play-list

*prev*
	Change to the previous track in the play-list

*status*
	Get the playback status. Equivalent to the *info %play\_status* action

*info* [format string]
	Print information about the current track. *format string* can include any of the following *FORMAT SPECIFIERS*.

# FORMAT SPECIFIERS

*%player\_name*
	prints the player name

*%track\_name*
	prints the track name

*%track\_number*
	prints the track number

*%track\_length*
	prints the track length (seconds)

*%artist\_name*
	prints the artist name

*%album\_name*
	prints the album name

*%album\_artist*
	prints the album artist

*%art\_url*
	prints the URL of the cover art image

*%play\_status*
	prints the playback status

*%shuffle*
	prints the shuffle mode

*%volume*
	prints the volume

*%loop\_status*
	prints the loop status

*%position*
	prints the song position (seconds)

*%bitrate*
	prints the track's bitrate

*%comment*
	prints the track's comment

*%full*
	prints all available information

# EXAMPLE

*mpris-ctl info %full*

```
Player name:	Spotify
Play status:	Playing
Track:		Song 2
Artist:		Blur
Album:		Blur: The Best Of
Album Artist:	Blur
Art URL:		https://open.spotify.com/image/7731a6d23ed9f82ea94e89b248450eb00f044a1b
Track:		2
Length:		121.88s
Volume:		0.00
Loop status:	None
Shuffle:		false
Position:		0.00s
Bitrate:		0
Comment:
```

# NOTES

1.  *MPRIS D-Bus Interface Specification*
	https://specifications.freedesktop.org/mpris-spec/latest/
