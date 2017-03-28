/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sdbus.h"

#define ARG_HELP        "help"
#define ARG_PLAY        "play"
#define ARG_PAUSE       "pause"
#define ARG_STOP        "stop"
#define ARG_NEXT        "next"
#define ARG_PREVIOUS    "prev"
#define ARG_PLAY_PAUSE  "pp"
#define ARG_STATUS      "status"

#define HELP_MESSAGE    "MPRIS control, version %s\n" \
"Usage:\n  %s COMMAND - Control running MPRIS player\n" \
"Commands:\n"\
"\t" ARG_HELP "\t\tThis help message\n" \
"\t" ARG_STATUS "\t\tGet the play status\n" \
"\t" ARG_PLAY_PAUSE "\t\tToggle play or pause\n" \
"\t" ARG_PAUSE "\t\tPause the player\n" \
"\t" ARG_STOP "\t\tStop the player\n" \
"\t" ARG_NEXT "\t\tChange track to the next in the playlist\n" \
"\t" ARG_PREVIOUS "\t\tChange track to the previous in the playlist\n"

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
    if (strcmp(command, ARG_STATUS) == 0) {
        return DBUS_PROPERTIES_INTERFACE;
    }

    return NULL;
}

void print_help(char* name)
{
    const char* help_msg;
    const char* version = get_version();

    help_msg = HELP_MESSAGE;

    fprintf(stdout, help_msg, version, name);
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

    char *dbus_method = (char*)get_dbus_method(command);
    if (NULL == dbus_method) {
        fprintf(stderr, "Invalid command %s (use help for help)\n", command);
        goto _error;
    }
    char *dbus_property = NULL;
    dbus_property = (char*)get_dbus_property_name(command);

    DBusConnection* conn;
    DBusError err;

    // initialise the errors
    dbus_error_init(&err);

    // connect to the system bus and check for errors
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection error(%s)\n", err.message);
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
        fprintf(stderr, "Name error(%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
        goto _error;
    }

    char* destination = get_player_name(conn);
    if (strlen(destination) == 0) { goto _error; }

    if (NULL == dbus_property) {
        call_dbus_method(conn, destination,
                         MPRIS_PLAYER_PATH,
                         MPRIS_PLAYER_INTERFACE,
                         dbus_method);
    } else {
        mpris_properties prop = get_mpris_properties(conn, destination);
        fprintf(stdout, "%s\n", prop.playback_status);
    }

    dbus_connection_flush(conn);

    _success:
    {
        return EXIT_SUCCESS;
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

