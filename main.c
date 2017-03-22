/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

#define LOCAL_NAME                 "org.mpris.mprisctl"
#define MPRIS_PLAYER_NAMESPACE     "org.mpris.MediaPlayer2"
#define MPRIS_PLAYER_PATH          "/org/mpris/MediaPlayer2"
#define MPRIS_PLAYER_INTERFACE     "org.mpris.MediaPlayer2.Player"
#define MPRIS_METHOD_NEXT          "Next"
#define MPRIS_METHOD_PREVIOUS      "Previous"
#define MPRIS_METHOD_PLAY          "Play"
#define MPRIS_METHOD_PAUSE         "Pause"
#define MPRIS_METHOD_STOP          "Stop"
#define MPRIS_METHOD_PLAY_PAUSE    "PlayPause"

#define DBUS_DESTINATION           "org.freedesktop.DBus"
#define DBUS_PATH                  "/"
#define DBUS_INTERFACE             "org.freedesktop.DBus"
#define DBUS_METHOD_LIST_NAMES     "ListNames"

#define ARG_HELP        "help"
#define ARG_PLAY        "play"
#define ARG_PAUSE       "pause"
#define ARG_STOP        "stop"
#define ARG_NEXT        "next"
#define ARG_PREVIOUS    "prev"
#define ARG_PLAY_PAUSE  "pp"

const char* get_version()
{
#ifdef VERSION_MAJOR
    const char* version = VERSION_MAJOR "." VERSION_MINOR "." VERSION_PATCH;
#else
#ifdef VERSION_HASH
    const char* version = VERSION_HASH;
#endif
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

DBusMessage* call_dbus_method(DBusConnection* conn, char* destination, char* path, char* interface, char* method) 
{
    DBusMessage* msg;
    DBusPendingCall* pending;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return NULL; }
    
    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, -1)) { // -1 is default timeout
        fprintf(stderr, "Out Of Memory!\n");
        return NULL;
    }
    if (NULL == pending) { 
        fprintf(stderr, "Pending Call Null\n");
        return NULL;
    }

    // free message
    dbus_message_unref(msg);

    // block until we receive a reply
    dbus_pending_call_block(pending);

    DBusMessage* reply;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);
    if (NULL == reply) {
        fprintf(stderr, "Reply Null\n"); 
    }

    // free the pending message handle
    dbus_pending_call_unref(pending);

    return reply;
}

char* get_player_name(DBusConnection* conn) {
    char* player_name;
    if (NULL == conn) { return NULL; }

    char* method = DBUS_METHOD_LIST_NAMES;
    char* destination = DBUS_DESTINATION;
    char* path = DBUS_PATH;
    char* interface = DBUS_INTERFACE;
    const char* mpris_namespace = MPRIS_PLAYER_NAMESPACE;
    DBusMessage* reply = call_dbus_method(conn, destination, path, interface, method); 
    if (NULL == reply) { return NULL; }

    DBusMessageIter rootIter;
    if (dbus_message_iter_init(reply, &rootIter) && 
        DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&rootIter)) {
        DBusMessageIter arrayElementIter;

        dbus_message_iter_recurse(&rootIter, &arrayElementIter); 
        while (dbus_message_iter_has_next(&arrayElementIter)) {
            if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&arrayElementIter)) {
                char* str;
                dbus_message_iter_get_basic(&arrayElementIter, &str);
                if (!strncmp(str, mpris_namespace, strlen(mpris_namespace))) {
                    player_name = str;
                    break;
                }
            } 
            dbus_message_iter_next(&arrayElementIter);
        }
    }

    char *ret = malloc(strlen(player_name));
    strcpy(ret, player_name);
    return ret;
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

    char* destination = get_player_name(conn);
    if (NULL == destination) { goto _error; }
    
    call_dbus_method(conn, destination, 
                           MPRIS_PLAYER_PATH, 
                           MPRIS_PLAYER_INTERFACE, 
                           dbus_method); 

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

