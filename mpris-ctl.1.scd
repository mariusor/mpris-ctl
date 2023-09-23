MPRIS-CTL(1)

# NAME

*mpris-ctl* - MPRIS media-player control utility

# SYNOPSIS

mpris-ctl [OPTIONS...] [COMMAND]

# DESCRIPTION

*mpris-ctl* is a control utility for any media-player that conforms to the *MPRIS D-Bus Interface Specification*[1]

# OPTIONS

*--player* active | inactive | <name ...>

	Specifies which player(s) the command should be addressed to. The valid values are:

	*active*
		all players that are currently in _Playing_ state.
	*inactive*
		all players that are currently in a _Stopped_ or _Paused_ state.
	*<name ...>*
		specific player names, separated by spaces.

To consider all players both active and inactive you must pass the the option
twice, once for each state: *--player active --player inactive*

# COMMANDS

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

*seek* [-][time][unit]
	Seek forwards in the track for given time. The time value can be a floating
	point number as understood by the current locale.

	The unit can be one of:  _s_: seconds (default), _ms_: milliseconds, _m_: minutes

	A negative value seeks backwards.

	The default is 5 seconds.

*status*
	Get the playback status. Equivalent to the *info %play\_status* action

*list*
	Get the name of the running player(s). Equivalent to the *info %player\_name* action

*info* [format string]
	Print information about the current track. *format string* can include any of the following *FORMAT SPECIFIERS*.

# FORMAT SPECIFIERS

*%player\_name*
	prints the name of the media-player

*%track\_name*
	prints the track name

*%track\_number*
	prints the track number

*%track\_length*
	prints the track length in seconds

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
	prints the volume level

*%loop\_status*
	prints the loop status

*%position*
	prints the position, in seconds, in the current track

*%bitrate*
	prints the track bit-rate

*%comment*
	prints the track comment

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
