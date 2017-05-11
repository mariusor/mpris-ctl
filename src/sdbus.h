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
#define MPRIS_ARG_PLAYER_IDENTITY  "Identity"

#define DBUS_DESTINATION           "org.freedesktop.DBus"
#define DBUS_PATH                  "/"
#define DBUS_INTERFACE             "org.freedesktop.DBus"
#define DBUS_PROPERTIES_INTERFACE  "org.freedesktop.DBus.Properties"
#define DBUS_METHOD_LIST_NAMES     "ListNames"
#define DBUS_METHOD_GET_ALL        "GetAll"
#define DBUS_METHOD_GET            "Get"

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
    uint64_t length; // mpris specific
    unsigned short track_number;
    unsigned short bitrate;
    unsigned short disc_number;

} mpris_metadata;

typedef struct mpris_properties {
    mpris_metadata metadata;
    double volume;
    int64_t position;
    char* player_name;
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

void mpris_metadata_init(mpris_metadata* metadata)
{
    metadata->track_number = 0;
    metadata->bitrate = 0;
    metadata->disc_number = 0;
    metadata->length = 0;
    metadata->album_artist = "unknown";
    metadata->composer = "unknown";
    metadata->genre = "unknown";
    metadata->artist = "unknown";
    metadata->comment = "";
    metadata->track_id = 0;
    metadata->album = "unknown";
    metadata->content_created = 0;
    metadata->title = "unknown";
    metadata->url = 0;
    metadata->art_url = 0;
}

void mpris_properties_init(mpris_properties *properties)
{
    mpris_metadata_init(&(properties->metadata));
    properties->volume = 0;
    properties->position = 0;
    properties->player_name = "unknown";
    properties->loop_status = "unknown";
    properties->playback_status = "unknown";
    properties->can_control = false;
    properties->can_go_next = false;
    properties->can_go_previous = false;
    properties->can_play = false;
    properties->can_pause = false;
    properties->can_seek = false;
    properties->shuffle = false;
}

void mpris_properties_unref(mpris_properties *properties)
{
    free(&(properties->metadata));
    free(properties);
}

DBusMessage* call_dbus_method(DBusConnection* conn, char* destination, char* path, char* interface, char* method)
{
    if (NULL == conn) { return NULL; }
    if (NULL == destination) { return NULL; }

    DBusMessage* msg;
    DBusPendingCall* pending;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return NULL; }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT)) {
        goto _unref_message_err;
    }
    if (NULL == pending) {
        goto _unref_message_err;
    }
    dbus_connection_flush(conn);

    // free message
    dbus_message_unref(msg);

    // block until we receive a reply
    dbus_pending_call_block(pending);

    DBusMessage* reply;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);

    // free the pending message handle
    dbus_pending_call_unref(pending);
    // free message
    dbus_message_unref(msg);

    return reply;

_unref_message_err:
    {
        dbus_message_unref(msg);
    }
    return NULL;
}

double extract_double_var(DBusMessageIter *iter, DBusError *error)
{
    double result = 0;

    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return 0;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_DOUBLE == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, &result);
        return result;
    }
    return 0;
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
    if (DBUS_TYPE_OBJECT_PATH == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, &result);
        return result;
    }
    if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, &result);
        return result;
    }
    if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&variantIter)) {
        DBusMessageIter arrayIter;
        dbus_message_iter_recurse(&variantIter, &arrayIter);
        while (true) {
            // todo(marius): load all elements of the array
            if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&arrayIter)) {
                dbus_message_iter_get_basic(&arrayIter, &result);
                return result;
            }
            if (!dbus_message_iter_has_next(&arrayIter)) {
                break;
            }
            dbus_message_iter_next(&arrayIter);
        }
    }
    return NULL;
}

int32_t extract_int32_var(DBusMessageIter *iter, DBusError *error)
{
    int32_t result = 0;
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return 0;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, &result);
        return result;
    }
    return 0;
}

uint64_t extract_uint64_var(DBusMessageIter *iter, DBusError *error)
{
    uint64_t result = 0;
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return 0;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_UINT64 == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, &result);
        return result;
    }
    return 0;
}

int64_t extract_int64_var(DBusMessageIter *iter, DBusError *error)
{
    int64_t result = 0;
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return 0;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_INT64 == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, &result);
        return result;
    }
    return 0;
}

bool extract_boolean_var(DBusMessageIter *iter,  DBusError *error)
{
    bool *result = false;

    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return false;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_BOOLEAN == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, &result);
        return result;
    }
    return false;
}

mpris_metadata load_metadata(DBusMessageIter *iter)
{
    mpris_metadata track;
    mpris_metadata_init(&track);

    DBusError err;
    dbus_error_init(&err);

    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(&err, "iter_should_be_variant", "This message iterator must be have variant type");
        return track;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_set_error_const(&err, "variant_should_be_array", "This variant reply message must have array content");
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
                dbus_set_error_const(&err, "missing_key", "This message iterator doesn't have key");
            }
            dbus_message_iter_get_basic(&dictIter, &key);

            if (!dbus_message_iter_has_next(&dictIter)) {
                continue;
            }
            dbus_message_iter_next(&dictIter);

            if (!strncmp(key, MPRIS_METADATA_BITRATE, strlen(MPRIS_METADATA_BITRATE))) {
                track.bitrate = extract_int32_var(&dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_ART_URL, strlen(MPRIS_METADATA_ART_URL))) {
                track.art_url = extract_string_var(&dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_LENGTH, strlen(MPRIS_METADATA_LENGTH))) {
                track.length = extract_uint64_var(&dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_TRACKID, strlen(MPRIS_METADATA_TRACKID))) {
                track.track_id = extract_string_var(&dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_ALBUM_ARTIST, strlen(MPRIS_METADATA_ALBUM_ARTIST))) {
                track.album_artist = extract_string_var(&dictIter, &err);
            } else if (!strncmp(key, MPRIS_METADATA_ALBUM, strlen(MPRIS_METADATA_ALBUM))) {
                track.album = extract_string_var(&dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_ARTIST, strlen(MPRIS_METADATA_ARTIST))) {
                track.artist = extract_string_var(&dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_COMMENT, strlen(MPRIS_METADATA_COMMENT))) {
                track.comment = extract_string_var(&dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_TITLE, strlen(MPRIS_METADATA_TITLE))) {
                track.title = extract_string_var(&dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_TRACK_NUMBER, strlen(MPRIS_METADATA_TRACK_NUMBER))) {
                track.track_number = extract_int32_var(&dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_URL, strlen(MPRIS_METADATA_URL))) {
                track.url = extract_string_var(&dictIter, &err);
            }
            if (dbus_error_is_set(&err)) {
                //fprintf(stderr, "err: %s, %s\n", key, err->message);
                dbus_error_free(&err);
            }
        }
        if (!dbus_message_iter_has_next(&arrayIter)) {
            break;
        }
        dbus_message_iter_next(&arrayIter);
    }
    return track;
}

char* get_player_identity(DBusConnection *conn, const char* destination)
{
    if (NULL == conn) { return NULL; }
    if (NULL == destination) { return NULL; }
    if (strncmp(MPRIS_PLAYER_NAMESPACE, destination, strlen(MPRIS_PLAYER_NAMESPACE))) { return NULL; }

    DBusMessage* msg;
    DBusError err;
    DBusPendingCall* pending;
    DBusMessageIter params;
    char* result = "unknown";

    char* interface = DBUS_PROPERTIES_INTERFACE;
    char* method = DBUS_METHOD_GET;
    char* path = MPRIS_PLAYER_PATH;
    char* arg_interface = MPRIS_PLAYER_NAMESPACE;
    char* arg_identity = MPRIS_ARG_PLAYER_IDENTITY;

    dbus_error_init(&err);
    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return NULL; }

    // append interface we want to get the property from
    dbus_message_iter_init_append(msg, &params);
    if (!dbus_message_iter_append_basic(&params, DBUS_TYPE_STRING, &arg_interface)) {
        goto _unref_message_err;
    }

    dbus_message_iter_init_append(msg, &params);
    if (!dbus_message_iter_append_basic(&params, DBUS_TYPE_STRING, &arg_identity)) {
        goto _unref_message_err;
    }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT)) {
        goto _unref_message_err;
    }
    if (NULL == pending) {
        goto _unref_message_err;
    }
    dbus_connection_flush(conn);

    // block until we receive a reply
    dbus_pending_call_block(pending);

    DBusMessage* reply;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);
    if (NULL == reply) { goto _unref_pending_err; }

    DBusMessageIter rootIter;
    if (dbus_message_iter_init(reply, &rootIter)) {
        result = extract_string_var(&rootIter, &err);
    }
    if (dbus_error_is_set(&err)) {
        dbus_error_free(&err);
    }

    dbus_message_unref(reply);
    // free the pending message handle
    dbus_pending_call_unref(pending);
    // free message
    dbus_message_unref(msg);

    return result;

_unref_pending_err:
    {
        dbus_pending_call_unref(pending);
        goto _unref_message_err;
    }
_unref_message_err:
    {
        dbus_message_unref(msg);
    }
    return NULL;
}

mpris_properties get_mpris_properties(DBusConnection* conn, const char* destination)
{
    mpris_properties properties;
    mpris_properties_init(&properties);

    if (NULL == conn) { return properties; }
    if (NULL == destination) { return properties; }

    DBusMessage* msg;
    DBusPendingCall* pending;
    DBusMessageIter params;
    DBusError err;

    dbus_error_init(&err);

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
        goto _unref_message_err;
    }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT)) {
        goto _unref_message_err;
    }
    if (NULL == pending) {
        goto _unref_message_err;
    }
    dbus_connection_flush(conn);
    // block until we receive a reply
    dbus_pending_call_block(pending);

    DBusMessage* reply;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);
    if (NULL == reply) {
        goto _unref_pending_err;
    }
    DBusMessageIter rootIter;
    if (dbus_message_iter_init(reply, &rootIter) && DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&rootIter)) {
        DBusMessageIter arrayElementIter;

        dbus_message_iter_recurse(&rootIter, &arrayElementIter);
        while (true) {
            char* key;
            if (DBUS_TYPE_DICT_ENTRY == dbus_message_iter_get_arg_type(&arrayElementIter)) {
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
                    properties.metadata = load_metadata(&dictIter);
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
                    //fprintf(stderr, "error: %s\n", err.message);
                    dbus_error_free(&err);
                }
            }
            if (!dbus_message_iter_has_next(&arrayElementIter)) {
                break;
            }
            dbus_message_iter_next(&arrayElementIter);
        }
    }
    dbus_message_unref(reply);
    // free the pending message handle
    dbus_pending_call_unref(pending);
    // free message
    dbus_message_unref(msg);

    properties.player_name = get_player_identity(conn, destination);
    return properties;

_unref_pending_err:
    {
        dbus_pending_call_unref(pending);
        goto _unref_message_err;
    }
_unref_message_err:
    {
        dbus_message_unref(msg);
    }
    return properties;
}

char* get_player_namespace(DBusConnection* conn)
{
    if (NULL == conn) { return NULL; }

    char* player_namespace = NULL;
    char* method = DBUS_METHOD_LIST_NAMES;
    char* destination = DBUS_DESTINATION;
    char* path = DBUS_PATH;
    char* interface = DBUS_INTERFACE;
    const char* mpris_namespace = MPRIS_PLAYER_NAMESPACE;

    DBusMessage* msg;
    DBusPendingCall* pending;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return NULL; }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT)) {
        goto _unref_message_err;
    }
    if (NULL == pending) {
        goto _unref_message_err;
    }
    dbus_connection_flush(conn);

    // block until we receive a reply
    dbus_pending_call_block(pending);

    DBusMessage* reply;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);
    if (NULL == reply) { goto _unref_pending_err; }

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
                    player_namespace = str;
                    break;
                }
            }
            if (!dbus_message_iter_has_next(&arrayElementIter)) {
                break;
            }
            dbus_message_iter_next(&arrayElementIter);
        }
    }
    dbus_message_unref(reply);
    // free the pending message handle
    dbus_pending_call_unref(pending);
    // free message
    dbus_message_unref(msg);

    return player_namespace;

_unref_pending_err:
    {
        dbus_pending_call_unref(pending);
        goto _unref_message_err;
    }
_unref_message_err:
    {
        dbus_message_unref(msg);
    }
    return NULL;
}
