/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "sstring.h"
#include "sdbus.h"

#define ARG_HELP        "help"
#define ARG_PLAY        "play"
#define ARG_PAUSE       "pause"
#define ARG_STOP        "stop"
#define ARG_NEXT        "next"
#define ARG_PREVIOUS    "prev"
#define ARG_PLAY_PAUSE  "pp"
#define ARG_STATUS      "status"
#define ARG_INFO        "info"

#define ARG_INFO_DEFAULT_STATUS "%track_name - %album_name - %artist_name"
#define ARG_INFO_FULL_STATUS    "Player name:\t" ARG_INFO_PLAYER_NAME "\n" \
"Play status:\t" ARG_INFO_PLAYBACK_STATUS "\n" \
"Track:\t\t" ARG_INFO_TRACK_NAME "\n" \
"Artist:\t\t" ARG_INFO_ARTIST_NAME "\n" \
"Album:\t\t" ARG_INFO_ALBUM_NAME "\n" \
"Album Artist:\t" ARG_INFO_ALBUM_ARTIST "\n" \
"Art URL:\t" ARG_INFO_ART_URL "\n" \
"Track:\t\t" ARG_INFO_TRACK_NUMBER "\n" \
"Length:\t\t" ARG_INFO_TRACK_LENGTH "\n" \
"Volume:\t\t" ARG_INFO_VOLUME "\n" \
"Loop status:\t" ARG_INFO_LOOP_STATUS "\n" \
"Shuffle:\t" ARG_INFO_SHUFFLE_MODE "\n" \
"Position:\t" ARG_INFO_POSITION "\n" \
"Bitrate:\t" ARG_INFO_BITRATE "\n" \
"Comment:\t" ARG_INFO_COMMENT \
""

#define ARG_INFO_PLAYER_NAME     "%player_name"
#define ARG_INFO_TRACK_NAME      "%track_name"
#define ARG_INFO_TRACK_NUMBER    "%track_number"
#define ARG_INFO_TRACK_LENGTH    "%track_length"
#define ARG_INFO_ARTIST_NAME     "%artist_name"
#define ARG_INFO_ALBUM_NAME      "%album_name"
#define ARG_INFO_ALBUM_ARTIST    "%album_artist"
#define ARG_INFO_ART_URL         "%art_url"
#define ARG_INFO_BITRATE         "%bitrate"
#define ARG_INFO_COMMENT         "%comment"

#define ARG_INFO_PLAYBACK_STATUS "%play_status"
#define ARG_INFO_SHUFFLE_MODE    "%shuffle"
#define ARG_INFO_VOLUME          "%volume"
#define ARG_INFO_LOOP_STATUS     "%loop_status"
#define ARG_INFO_POSITION        "%position"

#define ARG_INFO_FULL            "%full"

#define TRUE_LABEL      "true"
#define FALSE_LABEL     "false"

#define HELP_MESSAGE    "MPRIS control, version %s\n" \
"Usage:\n  %s COMMAND - Control running MPRIS player\n" \
"Commands:\n"\
"\t" ARG_HELP "\t\tThis help message\n" \
"\t" ARG_PLAY "\t\tBegin playing\n" \
"\t" ARG_PLAY_PAUSE "\t\tToggle play or pause\n" \
"\t" ARG_PAUSE "\t\tPause the player\n" \
"\t" ARG_STOP "\t\tStop the player\n" \
"\t" ARG_NEXT "\t\tChange track to the next in the playlist\n" \
"\t" ARG_PREVIOUS "\t\tChange track to the previous in the playlist\n" \
"\t" ARG_STATUS "\t\tGet the playback status\n" \
"\t\t\t- equivalent to " ARG_INFO " \"%s\"\n" \
"\t" ARG_INFO "\t\t<format> Display information about the current track\n" \
"\t\t\t- default value\"%s\"\n\n" \
"Format specifiers:\n" \
"\t%" ARG_INFO_PLAYER_NAME "\tprints the player name\n" \
"\t%" ARG_INFO_TRACK_NAME "\tprints the track name\n" \
"\t%" ARG_INFO_TRACK_NUMBER "\tprints the track number\n" \
"\t%" ARG_INFO_TRACK_LENGTH "\tprints the track length (useconds)\n" \
"\t%" ARG_INFO_ARTIST_NAME "\tprints the artist name\n" \
"\t%" ARG_INFO_ALBUM_NAME "\tprints the album name\n" \
"\t%" ARG_INFO_ALBUM_ARTIST "\tprints the album artist\n" \
"\t%" ARG_INFO_ART_URL "\tprints the URL of the cover art image\n" \
"\t%" ARG_INFO_PLAYBACK_STATUS "\tprints the playback status\n" \
"\t%" ARG_INFO_SHUFFLE_MODE "\tprints the shuffle mode\n" \
"\t%" ARG_INFO_VOLUME "\t\tprints the volume\n" \
"\t%" ARG_INFO_LOOP_STATUS "\tprints the loop status\n" \
"\t%" ARG_INFO_POSITION "\tprints the song position (useconds)\n" \
"\t%" ARG_INFO_BITRATE "\tprints the track's bitrate\n" \
"\t%" ARG_INFO_COMMENT "\tprints the track's comment\n" \
"\t%" ARG_INFO_FULL "\t\tprints all available information\n" \
""

const char* get_version()
{
#ifndef VERSION_HASH
#define VERSION_HASH "(unknown)"
#endif
    return VERSION_HASH;
}

const char* get_dbus_property_name (char* command)
{
    if (NULL == command) return NULL;
    if (strcmp(command, ARG_STATUS) == 0) {
        return MPRIS_PROP_PLAYBACK_STATUS;
    }
    if (strcmp(command, ARG_INFO) == 0) {
        return MPRIS_PROP_METADATA;
    }

    return NULL;
}

const char* get_dbus_method (char* command)
{
    if (NULL == command) return NULL;

    if (strcmp(command, ARG_PLAY) == 0) {
        return MPRIS_METHOD_PLAY;
    }
    if (strcmp(command,ARG_PAUSE) == 0) {
        return MPRIS_METHOD_PAUSE;
    }
    if (strcmp(command, ARG_STOP) == 0) {
        return MPRIS_METHOD_STOP;
    }
    if (strcmp(command, ARG_NEXT) == 0) {
        return MPRIS_METHOD_NEXT;
    }
    if (strcmp(command, ARG_PREVIOUS) == 0) {
        return MPRIS_METHOD_PREVIOUS;
    }
    if (strcmp(command, ARG_PLAY_PAUSE) == 0) {
        return MPRIS_METHOD_PLAY_PAUSE;
    }
    if (strcmp(command, ARG_STATUS) == 0 || strcmp(command, ARG_INFO) == 0) {
        return DBUS_PROPERTIES_INTERFACE;
    }

    return NULL;
}

void print_help(char* name)
{
    const char* help_msg;
    const char* version = get_version();

    help_msg = HELP_MESSAGE;
    char* info_def = ARG_INFO_DEFAULT_STATUS;
    char* status_def = ARG_INFO_PLAYBACK_STATUS;

    fprintf(stdout, help_msg, version, name, status_def, info_def);
}

void print_mpris_info(mpris_properties *props, char* format)
{
    const char* info_full = ARG_INFO_FULL_STATUS;
    const char* shuffle_label = (props->shuffle ? TRUE_LABEL : FALSE_LABEL);
    char* volume_label = get_zero_string(4);
    if (NULL == volume_label) { return; }
    snprintf(volume_label, 5, "%.2f", props->volume);
    char* pos_label = get_zero_string(10);
    if (NULL == pos_label) { goto error_pos_label; }
    snprintf(pos_label, 11, "%.2lfs", (props->position / 1000000.0));
    char* track_number_label = get_zero_string(6);
    if (NULL == track_number_label) { goto error_track_number_label; }
    snprintf(track_number_label, 6, "%d", props->metadata.track_number);
    char* bitrate_label = get_zero_string(6);
    if (NULL == bitrate_label) { goto error_bitrate_label; }
    snprintf(bitrate_label, 6, "%d", props->metadata.bitrate);
    char* length_label = get_zero_string(15);
    snprintf(length_label, 15, "%.2lfs", (props->metadata.length / 1000000.0));
    if (NULL == length_label) { goto error_length_label; }

    char* output = get_zero_string(MAX_OUTPUT_LENGTH);
    if (NULL == output) { goto error_output; }
    strncpy(output, format, MAX_OUTPUT_LENGTH);

    output = str_replace(output, "\\n", "\n");
    output = str_replace(output, "\\t", "\t");

    output = str_replace(output, ARG_INFO_FULL, info_full);

    output = str_replace(output, ARG_INFO_PLAYER_NAME, props->player_name);
    output = str_replace(output, ARG_INFO_SHUFFLE_MODE, shuffle_label);
    output = str_replace(output, ARG_INFO_PLAYBACK_STATUS, props->playback_status);
    output = str_replace(output, ARG_INFO_VOLUME, volume_label);
    output = str_replace(output, ARG_INFO_LOOP_STATUS, props->loop_status);
    output = str_replace(output, ARG_INFO_POSITION, pos_label);
    output = str_replace(output, ARG_INFO_TRACK_NAME, props->metadata.title);
    output = str_replace(output, ARG_INFO_ARTIST_NAME, props->metadata.artist);
    output = str_replace(output, ARG_INFO_ALBUM_ARTIST, props->metadata.album_artist);
    output = str_replace(output, ARG_INFO_ALBUM_NAME, props->metadata.album);
    output = str_replace(output, ARG_INFO_TRACK_LENGTH, length_label);
    output = str_replace(output, ARG_INFO_TRACK_NUMBER, track_number_label);
    output = str_replace(output, ARG_INFO_BITRATE, bitrate_label);
    output = str_replace(output, ARG_INFO_COMMENT, props->metadata.comment);
    output = str_replace(output, ARG_INFO_ART_URL, props->metadata.url);

    fprintf(stdout, "%s\n", output);
    free(output);
error_output:
    free(length_label);
error_length_label:
    free(bitrate_label);
error_bitrate_label:
    free(track_number_label);
error_track_number_label:
    free(pos_label);
error_pos_label:
    free(volume_label);
}

int main(int argc, char** argv)
{
    char* name = argv[0];
    if (argc <= 1) {
        goto _help;
    }

    char *command = argv[1];
    if (strcmp(command, ARG_HELP) == 0) {
        goto _help;
    }
    char *info_format = ARG_INFO_DEFAULT_STATUS;
    if (strcmp(command, ARG_INFO) == 0 && argc > 2) {
        info_format = argv[2];
    }
    if (strcmp(command, ARG_STATUS) == 0) {
        info_format = ARG_INFO_PLAYBACK_STATUS;
    }

    char *dbus_method = (char*)get_dbus_method(command);
    if (NULL == dbus_method) {
        //fprintf(stderr, "Invalid command %s (use help for help)\n", command);
        goto _error;
    }
    char *dbus_property = NULL;
    dbus_property = (char*)get_dbus_property_name(command);

    DBusConnection* conn;
    DBusError err;

    // initialise the errors
    dbus_error_init(&err);

    // connect to the system bus and check for errors
    conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        //fprintf(stderr, "Connection error(%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (NULL == conn) {
        goto _error;
    }

    // request a name on the bus
    int ret = dbus_bus_request_name(conn, LOCAL_NAME,
                               DBUS_NAME_FLAG_REPLACE_EXISTING,
                               &err);
    if (dbus_error_is_set(&err)) {
        //fprintf(stderr, "Name error(%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
        goto _dbus_error;
    }

    char* destination = get_player_namespace(conn);
    if (NULL == destination ) { goto _dbus_error; }
    if (strlen(destination) == 0) { goto _dbus_error; }

    if (NULL == dbus_property) {
        call_dbus_method(conn, destination,
                         MPRIS_PLAYER_PATH,
                         MPRIS_PLAYER_INTERFACE,
                         dbus_method);
    } else {
        mpris_properties properties = get_mpris_properties(conn, destination);
        print_mpris_info(&properties, info_format);
    }
    if (NULL != destination) { free(destination); }

    dbus_connection_close(conn);
    dbus_connection_unref(conn);
    _success:
    {
        return EXIT_SUCCESS;
    }
    _dbus_error: {
        dbus_connection_close(conn);
        dbus_connection_unref(conn);

        goto _error;
    }
    _error:
    {
        return EXIT_FAILURE;
    }
    _help:
    {
        print_help(name);
        goto _success;
    }
}

