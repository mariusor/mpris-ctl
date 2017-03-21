/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <glib.h>
//#include <dbus/dbus-glib.h>
#include <dbus/dbus.h>

#define DBUS_MPRIS_NAMESPACE		    "org.mpris.MediaPlayer2"
#define DBUS_LOCAL_PLAYER			    "org.freedesktop.Player"
#define DBUS_MPRIS_PLAYER_PATH		    "/org/mpris/MediaPlayer2/Player"
#define DBUS_MPRIS_PLAYER_INTERFACE     "org.mpris.MediaPlayer2.Player"
#define DBUS_MPRIS_METHOD_NEXT          "Next"
#define DBUS_MPRIS_METHOD_PREVIOUS      "Previous"
#define DBUS_MPRIS_METHOD_PLAY          "Play"
#define DBUS_MPRIS_METHOD_PAUSE         "Pause"
#define DBUS_MPRIS_METHOD_PLAY_PAUSE    "PlayPause"
#define DBUS_MPRIS_TRACK_SIGNAL		    "TrackChange"
#define DBUS_MPRIS_STATUS_SIGNAL	    "StatusChange"

//#define DBUS_TYPE_G_STRING_VALUE_HASHTABLE (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))

#define COMMAND_PLAY "play"
#define COMMAND_PAUSE "pause"

int main(int argc, char** argv) 
{
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], COMMAND_PLAY) == 0) {
            printf("playing\n");    
        }
        if (strcmp(argv[i],COMMAND_PAUSE) == 0) {
            printf("pausing\n");    
        }
    }

    DBusMessage* msg;
    DBusMessageIter args;
    DBusConnection* conn;
    DBusError err;
    DBusPendingCall* pending;

    // initialise the errors
    dbus_error_init(&err);

    // connect to the system bus and check for errors
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) { 
        fprintf(stderr, "Connection Error (%s)\n", err.message); 
        dbus_error_free(&err);
    }
    if (NULL == conn) { 
        exit(1); 
    }

    int ret;
    // request a name on the bus
    ret = dbus_bus_request_name(conn, DBUS_LOCAL_PLAYER, 
                               DBUS_NAME_FLAG_REPLACE_EXISTING,
                               &err);
    if (dbus_error_is_set(&err)) { 
        fprintf(stderr, "Name Error (%s)\n", err.message); 
        dbus_error_free(&err); 
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) { 
        exit(1);
    }
    // create a new method call and check for errors
    msg = dbus_message_new_method_call(DBUS_MPRIS_NAMESPACE, // target for the method call
                                      DBUS_MPRIS_PLAYER_PATH, // object to call on
                                      DBUS_MPRIS_PLAYER_INTERFACE, // interface to call on
                                      DBUS_MPRIS_METHOD_PAUSE); // method name
    if (NULL == msg) { 
        fprintf(stderr, "Message Null\n");
        exit(1);
    }
    
    char* param = "";
    // append arguments
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &param)) { 
        fprintf(stderr, "Out Of Memory!\n"); 
        exit(1);
    }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, -1)) { // -1 is default timeout
        fprintf(stderr, "Out Of Memory!\n"); 
        exit(1);
    }
    if (NULL == pending) { 
        fprintf(stderr, "Pending Call Null\n"); 
        exit(1); 
    }
    dbus_connection_flush(conn);

    // free message
    dbus_message_unref(msg);


	return 0;
}
