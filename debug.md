GetPlayers:
dbus-send --print-reply --dest=org.freedesktop.DBus / org.freedesktop.DBus.ListNames | grep org.mpris | cut -d\" -f2

GetCurrentPlaying:
dbus-send --print-reply --type=method_call --dest=org.mpris.MediaPlayer2.spotify /org/mpris/MediaPlayer2 org.freedesktop.DBus.Properties.Get string:org.mpris.MediaPlayer2.Player string:Metadata

GetStatus:
dbus-send --print-reply --type=method_call --dest=org.mpris.MediaPlayer2.spotify /org/mpris/MediaPlayer2 org.freedesktop.DBus.Properties.Get string:org.mpris.MediaPlayer2.Player string:PlaybackStatus

SetShuffle:
dbus-send --print-reply --type=method_call --dest=org.mpris.MediaPlayer2.ncspot /org/mpris/MediaPlayer2 org.freedesktop.DBus.Properties.Set string:org.mpris.MediaPlayer2.Player string:Shuffle variant:boolean:'true'

Seek:
dbus-send --print-reply --type=method_call --dest=org.mpris.MediaPlayer2.ncspot /org/mpris/MediaPlayer2 org.freedesktop.DBus.Properties.Seek string:org.mpris.MediaPlayer2.Player int64:10

