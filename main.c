/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

#define DBUS_LOCAL_PLAYER               "org.mpris.mprisctl"
#define DBUS_MPRIS_DESTINATION            "org.mpris.MediaPlayer2.spotify"
#define DBUS_MPRIS_PLAYER_PATH          "/org/mpris/MediaPlayer2"
#define DBUS_MPRIS_PLAYER_INTERFACE     "org.mpris.MediaPlayer2.Player"
#define DBUS_MPRIS_METHOD_NEXT          "Next"
#define DBUS_MPRIS_METHOD_PREVIOUS      "Previous"
#define DBUS_MPRIS_METHOD_PLAY          "Play"
#define DBUS_MPRIS_METHOD_PAUSE         "Pause"
#define DBUS_MPRIS_METHOD_STOP          "Stop"
#define DBUS_MPRIS_METHOD_PLAY_PAUSE    "PlayPause"

#define ARG_HELP        "help"
#define ARG_PLAY        "play"
#define ARG_PAUSE       "pause"
#define ARG_STOP        "stop"
#define ARG_NEXT        "next"
#define ARG_PREVIOUS    "prev"
#define ARG_PLAY_PAUSE  "pp"

char* get_version()
{
#ifdef USE_VERSION
    char* version = VERSION_MAJOR "." VERSION_MINOR "." VERSION_PATCH;
#else
    char* version = "(local-build)";
#endif
    return version;
}

char* get_dbus_method (char* command) 
{
    if (NULL == command) return NULL;

    char* dbus_method;

    if (strcmp(command, ARG_PLAY) == 0) {
        dbus_method = DBUS_MPRIS_METHOD_PLAY;
    }
    if (strcmp(command,ARG_PAUSE) == 0) {
        dbus_method = DBUS_MPRIS_METHOD_PAUSE;
    }
    if (strcmp(command, ARG_STOP) == 0) {
        dbus_method = DBUS_MPRIS_METHOD_STOP;
    }
    if (strcmp(command, ARG_NEXT) == 0) {
        dbus_method = DBUS_MPRIS_METHOD_NEXT;
    }
    if (strcmp(command, ARG_PREVIOUS) == 0) {
        dbus_method = DBUS_MPRIS_METHOD_PREVIOUS;
    }
    if (strcmp(command, ARG_PLAY_PAUSE) == 0) {
        dbus_method = DBUS_MPRIS_METHOD_PLAY_PAUSE;
    }

    return dbus_method;
}

void print_help(char* name)
{
    char* help_msg;
    char* version = get_version();

    help_msg = "MPRIS player control, version %s\n" \
               "Usage: %s [" ARG_PLAY "|" \
                ARG_PLAY_PAUSE "|" \
                ARG_PAUSE "|" \
                ARG_STOP "|" \
                ARG_NEXT "|" \
                ARG_PREVIOUS "]\n";

    fprintf(stdout, help_msg, version, name);
}

DBusPendingCall* call_dbus_method(DBusConnection* conn, char* destination, char* path, char* interface, char* method) 
{
    DBusMessage* msg;
    DBusPendingCall* pending;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, // target for the method call
                                      path, // object to call on
                                      interface, // interface to call on
                                      method); // method name
    if (NULL == msg) { 
        fprintf(stderr, "Message Null!");
        return NULL;
    }
    
    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, -1)) { // -1 is default timeout
        fprintf(stderr, "Out Of Memory!");
        return NULL;
    }
    if (NULL == pending) { 
        fprintf(stderr, "Pending Call Null");
        return NULL;
    }

    // free message
    dbus_message_unref(msg);
    return pending;
}


int main(int argc, char** argv) 
{
    char* name = argv[0];
    if (argc <= 1) {
        print_help(name);
        goto _success;
    }
    
    char *command = argv[1];
    if (strcmp(command, ARG_HELP) == 0) {
        print_help(name);
        goto _success;
    }

    char *dbus_method = get_dbus_method(command);
    //DBusMessageIter args;
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

    int ret;
    // request a name on the bus
    ret = dbus_bus_request_name(conn, DBUS_LOCAL_PLAYER, 
                               DBUS_NAME_FLAG_REPLACE_EXISTING,
                               &err);
    if (dbus_error_is_set(&err)) { 
        fprintf(stderr, "Name error(%s)\n", err.message);
        dbus_error_free(&err); 
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) { 
        goto _error;
    }

    DBusPendingCall* pending;
    pending = call_dbus_method(conn, DBUS_MPRIS_DESTINATION, // target for the method call
                           DBUS_MPRIS_PLAYER_PATH, // object to call on
                           DBUS_MPRIS_PLAYER_INTERFACE, // interface to call on
                           dbus_method); // method name

    if (NULL == pending) {
        //
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
}

