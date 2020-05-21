MPRIS-CTL(1)

# NAME

*mpris-ctl* - MPRIS media-player control utility

# SYNOPSIS

mpris-ctl [OPTIONS...] [COMMAND] [format string]

# DESCRIPTION

*mpris-ctl* is a control utility for any media-player that conforms to the *MPRIS D-Bus Interface Specification*[1]

# OPTIONS

*--player* active | inactive | all | <name,...>

	Specifies which players the command should be addressed to. The valid values are:

	*all*
		all players.
	*active*
		all players that are currently in _Playing_ state.
	*inactive*
		all players that are currently in _Stopped_ or _Paused_ state.
	<name...>
		specific player names, separated by spaces.

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

*list*
	Get the name of the running player(s). Equivalent to the *info %player\_name* action

*info* [format string]
	Print information about the current track. *format string* can include any of the following *FORMAT SPECIFIERS*.

# FORMAT SPECIFIERS

*%player\_name*
	prints the media-player's name

*%track\_name*
	prints the track's name

*%track\_number*
	prints the track number

*%track\_length*
	prints the track length in seconds

*%artist\_name*
	prints the artist's name

*%album\_name*
	prints the album's name

*%album\_artist*
	prints the album's artist

*%art\_url*
	prints the URL of the cover art image

*%play\_status*
	prints the playback status

*%shuffle*
	prints the shuffle mode

*%volume*
	prints the volume level

*%loop\_status*
	prints the loop status

*%position*
	prints the track's position in seconds

*%bitrate*
	prints the track's bit-rate

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
