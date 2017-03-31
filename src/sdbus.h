/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <stdio.h>
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

#define MPRIS_PNAME_PLAYBACKSTATUS "PlaybackStatus"
#define MPRIS_PNAME_CANCONTROL     "CanControl"
#define MPRIS_PNAME_CANGONEXT      "CanGoNext"
#define MPRIS_PNAME_CANGOPREVIOUS  "CanGoPrevious"
#define MPRIS_PNAME_CANPLAY        "CanPlay"
#define MPRIS_PNAME_CANPAUSE       "CanPause"
#define MPRIS_PNAME_CANSEEK        "CanSeek"
#define MPRIS_PNAME_SHUFFLE        "Shuffle"
#define MPRIS_PNAME_POSITION       "Position"
#define MPRIS_PNAME_VOLUME         "Volume"
#define MPRIS_PNAME_LOOPSTATUS     "LoopStatus"
#define MPRIS_PNAME_METADATA       "Metadata"

#define MPRIS_PROP_PLAYBACK_STATUS "PlaybackStatus"
#define MPRIS_PROP_METADATA        "Metadata"

#define DBUS_DESTINATION           "org.freedesktop.DBus"
#define DBUS_PATH                  "/"
#define DBUS_INTERFACE             "org.freedesktop.DBus"
#define DBUS_PROPERTIES_INTERFACE  "org.freedesktop.DBus.Properties"
#define DBUS_METHOD_LIST_NAMES     "ListNames"
#define DBUS_METHOD_GET_ALL        "GetAll"

#define MPRIS_METADATA_BITRATE      "bitrate"
#define MPRIS_METADATA_ART_URL      "mpris:artUrl"
#define MPRIS_METADATA_LENGTH       "mpris:length"
#define MPRIS_METADATA_TRACKID      "mpris:trackid"
#define MPRIS_METADATA_ALBUM        "xesam:album"
#define MPRIS_METADATA_ALBUM_ARTIST "xesam:albumArtist"
#define MPRIS_METADATA_ARTIST       "xesam:artist"
#define MPRIS_METADATA_COMMENT      "xesam:comment"
#define MPRIS_METADATA_TITLE        "xesam:title"
#define MPRIS_METADATA_TRACK_NUMBER "xesam:trackNumber"
#define MPRIS_METADATA_URL          "xesam:url"
#define MPRIS_METADATA_YEAR         "year"

// The default timeout leads to hangs when calling
//   certain players which don't seem to reply to MPRIS methods
#define DBUS_CONNECTION_TIMEOUT    100 //ms

typedef struct mpris_metadata {
    int track_number;
    int bitrate;
    int disc_number;
    int length; // mpris specific
    char* album_artist;
    char* composer;
    char* genre;
    char* artist;
    char* comment;
    char* track_id;
    char* album;
    char* content_created;
    char* title;
    char* url;
    char* art_url; //mpris specific

} mpris_metadata;

typedef struct mpris_properties {
    mpris_metadata metadata;
    double volume;
    int64_t position;
    char* loop_status;
    char* playback_status;
    bool can_control;
    bool can_go_next;
    bool can_go_previous;
    bool can_play;
    bool can_pause;
    bool can_seek;
    bool shuffle;
} mpris_properties;

DBusMessage* call_dbus_method(DBusConnection* conn, char* destination, char* path, char* interface, char* method)
{
    if (NULL == conn) { return NULL; }

    DBusMessage* msg;
    DBusPendingCall* pending;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return NULL; }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT)) {
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

double extract_double_var(DBusMessageIter *iter, DBusError *error)
{
    double result;

    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return 0;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_DOUBLE != dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_set_error_const(error, "variant_should_be_double", "This variant reply message must have double content");
        return 0;
    }
    dbus_message_iter_get_basic(&variantIter, &result);
    return result;
}

char* extract_string_var(DBusMessageIter *iter, DBusError *error)
{
    char* result;

    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return NULL;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_set_error_const(error, "variant_should_be_string", "This variant reply message must have string content");
        return NULL;
    }
    dbus_message_iter_get_basic(&variantIter, &result);
    return result;
}

int32_t extract_int32_var(DBusMessageIter *iter, DBusError *error)
{
    int32_t result;
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return 0;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_set_error_const(error, "variant_should_be_int32", "This variant reply message must have int32 content");
        return 0;
    }
    dbus_message_iter_get_basic(&variantIter, &result);
    return result;
}

int64_t extract_int64_var(DBusMessageIter *iter, DBusError *error)
{
    int64_t result;
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return 0;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_INT64 != dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_set_error_const(error, "variant_should_be_int64", "This variant reply message must have int64 content");
        return 0;
    }
    dbus_message_iter_get_basic(&variantIter, &result);
    return result;
}

bool extract_boolean_var(DBusMessageIter *iter,  DBusError *error)
{
    bool *result;

    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return false;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_BOOLEAN != dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_set_error_const(error, "variant_should_be_boolean", "This variant reply message must have boolean content");
        return false;
    }
    dbus_message_iter_get_basic(&variantIter, &result);
    return result;
}

#define MAX_STRING 100
#define MAX_COUNT 10
char* extract_string_array_var(DBusMessageIter *iter, DBusError *error)
{
    char* result;
    //char **result = malloc(sizeof(char*) * MAX_STRING * MAX_COUNT);
    //if (!result)
    //    return NULL;

    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return NULL;
    }
    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_set_error_const(error, "variant_should_be_array", "This variant reply message must have array content");
        return NULL;
    }
    DBusMessageIter arrayIter;
    dbus_message_iter_recurse(&variantIter, &arrayIter);
    //size_t arr_cnt = 0;
    while (true) {
        if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&arrayIter)) {
            dbus_message_iter_get_basic(&arrayIter, &result);
            return result;
            break;
        }
        if (!dbus_message_iter_has_next(&arrayIter)) {
            break;
        }
        dbus_message_iter_next(&arrayIter);
    }
    return NULL;
}

mpris_metadata load_metadata(DBusMessageIter *iter,  DBusError *error)
{
    mpris_metadata track = {};

    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return track;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_set_error_const(error, "variant_should_be_array", "This variant reply message must have array content");
        return track;
    }
    DBusMessageIter arrayIter;
    dbus_message_iter_recurse(&variantIter, &arrayIter);
    while (true) {
        char* key;
        if (DBUS_TYPE_DICT_ENTRY == dbus_message_iter_get_arg_type(&arrayIter)) {
            DBusMessageIter dictIter;
            dbus_message_iter_recurse(&arrayIter, &dictIter);
            if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&dictIter)) {
                dbus_set_error_const(error, "missing_key", "This message iterator doesn't have key");
            }
            dbus_message_iter_get_basic(&dictIter, &key);

            if (!dbus_message_iter_has_next(&dictIter)) {
                continue;
            }
            dbus_message_iter_next(&dictIter);

            if (!strncmp(key, MPRIS_METADATA_BITRATE, strlen(MPRIS_METADATA_BITRATE))) {
                track.bitrate = extract_int32_var(&dictIter, error);
            }
            if (!strncmp(key, MPRIS_METADATA_ART_URL, strlen(MPRIS_METADATA_ART_URL))) {
                track.art_url = extract_string_var(&dictIter, error);
            }
            if (!strncmp(key, MPRIS_METADATA_LENGTH, strlen(MPRIS_METADATA_LENGTH))) {
                track.length = extract_int64_var(&dictIter, error);
            }
            if (!strncmp(key, MPRIS_METADATA_TRACKID, strlen(MPRIS_METADATA_TRACKID))) {
                track.track_id = extract_string_var(&dictIter, error);
            }
            if (!strncmp(key, MPRIS_METADATA_ALBUM_ARTIST, strlen(MPRIS_METADATA_ALBUM_ARTIST))) {
                track.album_artist = extract_string_array_var(&dictIter, error);
            } else if (!strncmp(key, MPRIS_METADATA_ALBUM, strlen(MPRIS_METADATA_ALBUM))) {
                track.album = extract_string_var(&dictIter, error);
            }
            if (!strncmp(key, MPRIS_METADATA_ARTIST, strlen(MPRIS_METADATA_ARTIST))) {
                track.artist = extract_string_array_var(&dictIter, error);
            }
            if (!strncmp(key, MPRIS_METADATA_COMMENT, strlen(MPRIS_METADATA_COMMENT))) {
                track.comment = extract_string_array_var(&dictIter, error);
            }
            if (!strncmp(key, MPRIS_METADATA_TITLE, strlen(MPRIS_METADATA_TITLE))) {
                track.title = extract_string_var(&dictIter, error);
            }
            if (!strncmp(key, MPRIS_METADATA_TRACK_NUMBER, strlen(MPRIS_METADATA_TRACK_NUMBER))) {
                track.track_number = extract_int32_var(&dictIter, error);
            }
            if (!strncmp(key, MPRIS_METADATA_URL, strlen(MPRIS_METADATA_URL))) {
                track.url = extract_string_var(&dictIter, error);
            }
            if (dbus_error_is_set(error)) {
                fprintf(stderr, "error: %s\n", error->message);
                dbus_error_free(error);
            }
        }
        if (!dbus_message_iter_has_next(&arrayIter)) {
            break;
        }
        dbus_message_iter_next(&arrayIter);
    }
    return track;
}

mpris_properties get_mpris_properties(DBusConnection* conn, char* destination)
{
    mpris_properties properties = {};
    properties.playback_status = "unknown";
    if (NULL == conn) { return properties; }

    DBusMessage* msg;
    DBusPendingCall* pending;
    DBusMessageIter params;

    char* interface = DBUS_PROPERTIES_INTERFACE;
    char* method = DBUS_METHOD_GET_ALL;
    char* path = MPRIS_PLAYER_PATH;
    char* arg_interface = MPRIS_PLAYER_INTERFACE;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return properties; }

    // append interface we want to get the property from
    dbus_message_iter_init_append(msg, &params);
    if (!dbus_message_iter_append_basic(&params, DBUS_TYPE_STRING, &arg_interface)) {
        fprintf(stderr, "Out Of Memory!\n");
    }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT)) {
        fprintf(stderr, "Out Of Memory!\n");
    }
    if (NULL == pending) {
        fprintf(stderr, "Pending Call Null\n");
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

    DBusMessageIter rootIter;
    if (dbus_message_iter_init(reply, &rootIter) && DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&rootIter)) {
        DBusMessageIter arrayElementIter;

        dbus_message_iter_recurse(&rootIter, &arrayElementIter);
        while (true) {
            char* key;
            if (DBUS_TYPE_DICT_ENTRY == dbus_message_iter_get_arg_type(&arrayElementIter)) {
                DBusError err = {};
                DBusMessageIter dictIter;
                dbus_message_iter_recurse(&arrayElementIter, &dictIter);
                if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&dictIter)) {
                    dbus_set_error_const(&err, "missing_key", "This message iterator doesn't have key");
                }
                dbus_message_iter_get_basic(&dictIter, &key);

                if (!dbus_message_iter_has_next(&dictIter)) {
                    continue;
                }
                dbus_message_iter_next(&dictIter);

                if (!strncmp(key, MPRIS_PNAME_CANCONTROL, strlen(MPRIS_PNAME_CANCONTROL))) {
                     properties.can_control = extract_boolean_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_CANGONEXT, strlen(MPRIS_PNAME_CANGONEXT))) {
                     properties.can_go_next = extract_boolean_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_CANGOPREVIOUS, strlen(MPRIS_PNAME_CANGOPREVIOUS))) {
                   properties.can_go_previous = extract_boolean_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_CANPAUSE, strlen(MPRIS_PNAME_CANPAUSE))) {
                    properties.can_pause = extract_boolean_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_CANPLAY, strlen(MPRIS_PNAME_CANPLAY))) {
                    properties.can_play = extract_boolean_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_CANSEEK, strlen(MPRIS_PNAME_CANSEEK))) {
                    properties.can_seek = extract_boolean_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_LOOPSTATUS, strlen(MPRIS_PNAME_LOOPSTATUS))) {
                    properties.loop_status = extract_string_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_METADATA, strlen(MPRIS_PNAME_METADATA))) {
                    properties.metadata = load_metadata(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_PLAYBACKSTATUS, strlen(MPRIS_PNAME_PLAYBACKSTATUS))) {
                     properties.playback_status = extract_string_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_POSITION, strlen(MPRIS_PNAME_POSITION))) {
                      properties.position= extract_int64_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_SHUFFLE, strlen(MPRIS_PNAME_SHUFFLE))) {
                    properties.shuffle = extract_boolean_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_VOLUME, strlen(MPRIS_PNAME_VOLUME))) {
                     properties.volume = extract_double_var(&dictIter, &err);
                }
                if (dbus_error_is_set(&err)) {
                    fprintf(stderr, "error: %s\n", err.message);
                    dbus_error_free(&err);
                }
            }
            if (!dbus_message_iter_has_next(&arrayElementIter)) {
                break;
            }
            dbus_message_iter_next(&arrayElementIter);
        }
    }

    return properties;
}

char* get_dbus_string_scalar(DBusMessage* message)
{
    if (NULL == message) { return NULL; }
    char* status;

    DBusMessageIter rootIter;
    if (dbus_message_iter_init(message, &rootIter) &&
        DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&rootIter)) {

        dbus_message_iter_get_basic(&rootIter, &status);
    }

    return status;
}

char* get_player_name(DBusConnection* conn)
{
    if (NULL == conn) { return NULL; }

    char* player_name = "";

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
        while (true) {
            if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&arrayElementIter)) {
                char* str;
                dbus_message_iter_get_basic(&arrayElementIter, &str);
                if (!strncmp(str, mpris_namespace, strlen(mpris_namespace))) {
                    player_name = str;
                    break;
                }
            }
            if (!dbus_message_iter_has_next(&arrayElementIter)) {
                break;
            }
            dbus_message_iter_next(&arrayElementIter);
        }
    }

    return player_name;
}
