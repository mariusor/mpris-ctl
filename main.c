/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

#define LOCAL_NAME               "org.mpris.mprisctl"
#define MPRIS_DESTINATION          "org.mpris.MediaPlayer2"
#define MPRIS_PLAYER_PATH          "/org/mpris/MediaPlayer2"
#define MPRIS_PLAYER_INTERFACE     "org.mpris.MediaPlayer2.Player"
#define MPRIS_METHOD_NEXT          "Next"
#define MPRIS_METHOD_PREVIOUS      "Previous"
#define MPRIS_METHOD_PLAY          "Play"
#define MPRIS_METHOD_PAUSE         "Pause"
#define MPRIS_METHOD_STOP          "Stop"
#define MPRIS_METHOD_PLAY_PAUSE    "PlayPause"

#define ARG_HELP        "help"
#define ARG_PLAY        "play"
#define ARG_PAUSE       "pause"
#define ARG_STOP        "stop"
#define ARG_NEXT        "next"
#define ARG_PREVIOUS    "prev"
#define ARG_PLAY_PAUSE  "pp"

const char* get_version()
{
#ifdef USE_VERSION
    const char* version = VERSION_MAJOR "." VERSION_MINOR "." VERSION_PATCH;
#else
    const char* version = "(local-build)";
#endif
    return version;
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

    return NULL;
}

void print_help(char* name)
{
    const char* help_msg;
    const char* version = get_version();

    help_msg = "MPRIS player control, version %s\n" \
               "Usage: %s [" ARG_PLAY "|" \
                ARG_PLAY_PAUSE "|" \
                ARG_PAUSE "|" \
                ARG_STOP "|" \
                ARG_NEXT "|" \
                ARG_PREVIOUS "]\n";

    fprintf(stdout, help_msg, version, name);
}

char* get_player_name(DBusConnection* conn) {
    char* player_name;
    if (NULL == conn) { return NULL; }

    

    player_name = ".spotify";
    char *ret = malloc(strlen(player_name));
    strcpy(ret, player_name);
    return ret;
}

char* get_dbus_destination(DBusConnection* conn) 
{
    const char* mpris_namespace = MPRIS_DESTINATION;
    char* player_name = get_player_name(conn);
    char* full_name = malloc(strlen(mpris_namespace)+strlen(player_name)+1);

    strcpy(full_name, mpris_namespace);
    strcat(full_name, player_name);

    return full_name;
}

DBusPendingCall* call_dbus_method(DBusConnection* conn, char* destination, char* path, char* interface, char* method) 
{
    DBusMessage* msg;
    DBusPendingCall* pending;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
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

    char *dbus_method = (char*)get_dbus_method(command);
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

    DBusPendingCall* pending;
    char* destination = get_dbus_destination(conn);
    
    pending = call_dbus_method(conn, destination, // target for the method call
                           MPRIS_PLAYER_PATH, // object to call on
                           MPRIS_PLAYER_INTERFACE, // interface to call on
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

