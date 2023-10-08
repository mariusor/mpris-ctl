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
#define MPRIS_METHOD_SEEK          "Seek"

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
#define DBUS_METHOD_SET            "Set"

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

#define MPRIS_METADATA_VALUE_STOPPED "Stopped"
#define MPRIS_METADATA_VALUE_PLAYING "Playing"
#define MPRIS_METADATA_VALUE_PAUSED  "Paused"

#define MPRIS_LOOPSTATUS_VALUE_NONE      "None"
#define MPRIS_LOOPSTATUS_VALUE_TRACK     "Track"
#define MPRIS_LOOPSTATUS_VALUE_PLAYLIST  "Playlist"

// The default timeout leads to hangs when calling
//   certain players which don't seem to reply to MPRIS methods
#define DBUS_CONNECTION_TIMEOUT    100 //ms

#define MAX_PLAYERS 20

typedef struct mpris_metadata {
    uint64_t length; // mpris specific
    unsigned short track_number;
    unsigned short bitrate;
    unsigned short disc_number;
    char album_artist[MAX_OUTPUT_LENGTH];
    char composer[MAX_OUTPUT_LENGTH];
    char genre[MAX_OUTPUT_LENGTH];
    char artist[MAX_OUTPUT_LENGTH];
    char comment[MAX_OUTPUT_LENGTH];
    char track_id[MAX_OUTPUT_LENGTH];
    char album[MAX_OUTPUT_LENGTH];
    char content_created[MAX_OUTPUT_LENGTH];
    char title[MAX_OUTPUT_LENGTH];
    char url[MAX_OUTPUT_LENGTH];
    char art_url[MAX_OUTPUT_LENGTH]; //mpris specific

} mpris_metadata;

typedef struct mpris_properties {
    double volume;
    uint64_t position;
    bool can_control;
    bool can_go_next;
    bool can_go_previous;
    bool can_play;
    bool can_pause;
    bool can_seek;
    bool shuffle;
    char player_name[MAX_OUTPUT_LENGTH];
    char loop_status[MAX_OUTPUT_LENGTH];
    char playback_status[MAX_OUTPUT_LENGTH];
    mpris_metadata metadata;
} mpris_properties;

typedef struct mpris_player {
    char *name;
    char namespace[MAX_OUTPUT_LENGTH];
    mpris_properties properties;
    bool skip;
} mpris_player;

void mpris_metadata_init(mpris_metadata* metadata)
{
    metadata->track_number = 0;
    metadata->bitrate = 0;
    metadata->disc_number = 0;
    metadata->length = 0;
    memcpy(metadata->album_artist, "unknown", 8);
    memcpy(metadata->composer, "unknown", 8);
    memcpy(metadata->genre, "unknown", 8);
    memcpy(metadata->artist, "unknown", 8);
    memcpy(metadata->album, "unknown", 8);
    memcpy(metadata->title, "unknown", 8);
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

void extract_string_var(char result[MAX_OUTPUT_LENGTH], DBusMessageIter *iter, DBusError *error)
{
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return;
    }
    memset(result, 0, MAX_OUTPUT_LENGTH);

    DBusMessageIter variantIter = {0};
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_OBJECT_PATH == dbus_message_iter_get_arg_type(&variantIter)) {
        char *val = NULL;
        dbus_message_iter_get_basic(&variantIter, &val);
        memcpy(result, val, strlen(val));
    } else if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&variantIter)) {
        char *val = NULL;
        dbus_message_iter_get_basic(&variantIter, &val);
        memcpy(result, val, strlen(val));
    } else if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&variantIter)) {
        DBusMessageIter arrayIter;
        dbus_message_iter_recurse(&variantIter, &arrayIter);
        while (true) {
            // todo(marius): load all elements of the array
            if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&arrayIter)) {
                char *val = NULL;
                dbus_message_iter_get_basic(&arrayIter, &val);
                strncat(result, val, MAX_OUTPUT_LENGTH - strlen(result) - 1);
            }
            if (!dbus_message_iter_has_next(&arrayIter)) {
                break;
            } else {
                strncat(result, ", ", 3);
            }
            dbus_message_iter_next(&arrayIter);
        }
    }
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
    if (DBUS_TYPE_UINT64 == dbus_message_iter_get_arg_type(&variantIter)) {
        uint64_t temp;
        dbus_message_iter_get_basic(&variantIter, &temp);
        result = (int64_t)temp;
        return result;
    }
    return 0;
}

bool extract_boolean_var(DBusMessageIter *iter, DBusError *error)
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

void load_metadata(mpris_metadata *track, DBusMessageIter *iter)
{
    DBusError err = {0};
    dbus_error_init(&err);

    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(&err, "iter_should_be_variant", "This message iterator must be have variant type");
        return;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_set_error_const(&err, "variant_should_be_array", "This variant reply message must have array content");
        return;
    }
    DBusMessageIter arrayIter;
    dbus_message_iter_recurse(&variantIter, &arrayIter);
    while (true) {
        char* key = NULL;
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
                track->bitrate = extract_int32_var(&dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_ART_URL, strlen(MPRIS_METADATA_ART_URL))) {
                extract_string_var(track->art_url, &dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_LENGTH, strlen(MPRIS_METADATA_LENGTH))) {
                track->length = extract_int64_var(&dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_TRACKID, strlen(MPRIS_METADATA_TRACKID))) {
                extract_string_var(track->track_id, &dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_ALBUM_ARTIST, strlen(MPRIS_METADATA_ALBUM_ARTIST))) {
                extract_string_var(track->album_artist, &dictIter, &err);
            } else if (!strncmp(key, MPRIS_METADATA_ALBUM, strlen(MPRIS_METADATA_ALBUM))) {
                extract_string_var(track->album, &dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_ARTIST, strlen(MPRIS_METADATA_ARTIST))) {
                extract_string_var(track->artist, &dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_COMMENT, strlen(MPRIS_METADATA_COMMENT))) {
                extract_string_var(track->comment, &dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_TITLE, strlen(MPRIS_METADATA_TITLE))) {
                extract_string_var(track->title, &dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_TRACK_NUMBER, strlen(MPRIS_METADATA_TRACK_NUMBER))) {
                track->track_number = extract_int32_var(&dictIter, &err);
            }
            if (!strncmp(key, MPRIS_METADATA_URL, strlen(MPRIS_METADATA_URL))) {
                extract_string_var(track->url, &dictIter, &err);
            }
            if (dbus_error_is_set(&err)) {
                fprintf(stderr, "error: %s, %s\n", key, err.message);
                dbus_error_free(&err);
            }
        }
        if (!dbus_message_iter_has_next(&arrayIter)) {
            break;
        }
        dbus_message_iter_next(&arrayIter);
    }
}

void get_player_identity(char *identity, DBusConnection *conn, const char* destination)
{
    if (NULL == conn) { return; }
    if (NULL == destination) { return; }
    if (NULL == identity) { return; }
    if (strncmp(MPRIS_PLAYER_NAMESPACE, destination, strlen(MPRIS_PLAYER_NAMESPACE))) { return; }

    DBusMessage* msg;
    DBusPendingCall* pending;
    DBusMessageIter params;

    char *interface = DBUS_PROPERTIES_INTERFACE;
    char *method = DBUS_METHOD_GET;
    char *path = MPRIS_PLAYER_PATH;
    char *arg_interface = MPRIS_PLAYER_NAMESPACE;
    char *arg_identity = MPRIS_ARG_PLAYER_IDENTITY;

    DBusError err = {0};
    dbus_error_init(&err);

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return; }

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
        extract_string_var(identity, &rootIter, &err);
    }
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "error: %s\n", err.message);
        dbus_error_free(&err);
    }

    dbus_message_unref(reply);
_unref_pending_err:
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "error: %s\n", err.message);
        dbus_error_free(&err);
    }

    // free the pending message handle
    dbus_pending_call_unref(pending);
_unref_message_err:
    // free message
    dbus_message_unref(msg);
}

void load_mpris_properties(DBusConnection* conn, const char* destination, mpris_properties *properties)
{
    if (NULL == conn) { return; }
    if (NULL == destination) { return; }

    DBusMessage* msg;
    DBusPendingCall* pending;
    DBusMessageIter params;

    DBusError err = {0};
    dbus_error_init(&err);

    char* interface = DBUS_PROPERTIES_INTERFACE;
    char* method = DBUS_METHOD_GET_ALL;
    char* path = MPRIS_PLAYER_PATH;
    char* arg_interface = MPRIS_PLAYER_INTERFACE;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return; }

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
                    properties->can_control = extract_boolean_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_CANGONEXT, strlen(MPRIS_PNAME_CANGONEXT))) {
                    properties->can_go_next = extract_boolean_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_CANGOPREVIOUS, strlen(MPRIS_PNAME_CANGOPREVIOUS))) {
                    properties->can_go_previous = extract_boolean_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_CANPAUSE, strlen(MPRIS_PNAME_CANPAUSE))) {
                    properties->can_pause = extract_boolean_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_CANPLAY, strlen(MPRIS_PNAME_CANPLAY))) {
                    properties->can_play = extract_boolean_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_CANSEEK, strlen(MPRIS_PNAME_CANSEEK))) {
                    properties->can_seek = extract_boolean_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_LOOPSTATUS, strlen(MPRIS_PNAME_LOOPSTATUS))) {
                    extract_string_var(properties->loop_status, &dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_METADATA, strlen(MPRIS_PNAME_METADATA))) {
                    load_metadata(&properties->metadata, &dictIter);
                }
                if (!strncmp(key, MPRIS_PNAME_PLAYBACKSTATUS, strlen(MPRIS_PNAME_PLAYBACKSTATUS))) {
                    extract_string_var(properties->playback_status, &dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_POSITION, strlen(MPRIS_PNAME_POSITION))) {
                    properties->position= extract_int64_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_SHUFFLE, strlen(MPRIS_PNAME_SHUFFLE))) {
                    properties->shuffle = extract_boolean_var(&dictIter, &err);
                }
                if (!strncmp(key, MPRIS_PNAME_VOLUME, strlen(MPRIS_PNAME_VOLUME))) {
                    properties->volume = extract_double_var(&dictIter, &err);
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
    dbus_message_unref(reply);
    // free the pending message handle
    dbus_pending_call_unref(pending);
    // free message
    dbus_message_unref(msg);

    get_player_identity(properties->player_name, conn, destination);
    return;

_unref_pending_err:
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "error: %s\n", err.message);
        dbus_error_free(&err);
    }
    {
        dbus_pending_call_unref(pending);
        goto _unref_message_err;
    }
_unref_message_err:
    {
        dbus_message_unref(msg);
    }
    return;
}

int seek(DBusConnection* conn, mpris_player player, int ms)
{
    if (NULL == conn) { return 0; }
    int status = 0;

    DBusMessage* msg;
    DBusPendingCall* pending;
    DBusMessageIter args;

    DBusError err = {0};
    dbus_error_init(&err);

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(player.namespace, MPRIS_PLAYER_PATH, MPRIS_PLAYER_INTERFACE, MPRIS_METHOD_SEEK);
    if (NULL == msg) { return status; }

    int64_t usec = ms * 1000;
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT64, &usec)) {
        status = -1;
        goto _unref_message_err;
    }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT) || NULL == pending) {
        status = -1;
        goto _unref_message_err;
    }
    dbus_connection_flush(conn);

    // block until we receive a reply
    dbus_pending_call_block(pending);

    DBusMessage* reply = NULL;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);
    if (NULL == reply) {
        status = -1;
        goto _unref_pending_err;
    }

    dbus_message_unref(reply);
_unref_pending_err:
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "error: %s\n", err.message);
        dbus_error_free(&err);
        status = -1;
    }

    // free the pending message handle
    dbus_pending_call_unref(pending);
_unref_message_err:
    // free message
    dbus_message_unref(msg);

    return status;
}

int shuffle(DBusConnection* conn, mpris_player player, bool state)
{
    if (NULL == conn) { return 0; }
    int status = 0;

    DBusError err = {0};
    dbus_error_init(&err);

    DBusMessage* msg;
    DBusPendingCall* pending;
    DBusMessageIter args;

    char* arg_interface = MPRIS_PLAYER_INTERFACE;
    char* arg_shuffle = MPRIS_PNAME_SHUFFLE;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(player.namespace, MPRIS_PLAYER_PATH, DBUS_PROPERTIES_INTERFACE, DBUS_METHOD_SET);
    if (NULL == msg) { return status; }

    dbus_message_iter_init_append(msg, &args);

    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &arg_interface)) {
        goto _unref_message_err;
    }

    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &arg_shuffle)) {
        goto _unref_message_err;
    }

    DBusMessageIter variant = {0};
    if (!dbus_message_iter_open_container(&args, DBUS_TYPE_VARIANT, DBUS_TYPE_BOOLEAN_AS_STRING, &variant)) {
        goto _unref_message_err;
    }
    if (!dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &state)) {
        goto _unref_message_err;
    }
    if (!dbus_message_iter_close_container(&args, &variant)) {
        goto _unref_message_err;
    }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT) || NULL == pending) {
        goto _unref_message_err;
    }
    dbus_connection_flush(conn);

    // block until we receive a reply
    dbus_pending_call_block(pending);

    DBusMessage* reply = NULL;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);
    if (NULL == reply) {
        goto _unref_pending_err;
    }

    dbus_message_unref(reply);
_unref_pending_err:
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "error: %s\n", err.message);
        dbus_error_free(&err);
        status = -1;
    }

    // free the pending message handle
    dbus_pending_call_unref(pending);
_unref_message_err:
    // free message
    dbus_message_unref(msg);

    return status;
}

int set_loopstatus(DBusConnection* conn, mpris_player player, const char* loop_state)
{
    if (NULL == conn) { return 0; }
    int status = 0;

    DBusError err = {0};
    dbus_error_init(&err);

    DBusMessage* msg;
    DBusPendingCall* pending;
    DBusMessageIter args;

    char* arg_interface = MPRIS_PLAYER_INTERFACE;
    char* arg_loopstatus = MPRIS_PNAME_LOOPSTATUS;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(player.namespace, MPRIS_PLAYER_PATH, DBUS_PROPERTIES_INTERFACE, DBUS_METHOD_SET);
    if (NULL == msg) { return status; }

    dbus_message_iter_init_append(msg, &args);

    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &arg_interface)) {
        goto _unref_message_err;
    }

    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &arg_loopstatus)) {
        goto _unref_message_err;
    }

    DBusMessageIter variant = {0};
    if (!dbus_message_iter_open_container(&args, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &variant)) {
        goto _unref_message_err;
    }
    if (!dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &loop_state)) {
        goto _unref_message_err;
    }
    if (!dbus_message_iter_close_container(&args, &variant)) {
        goto _unref_message_err;
    }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT) || NULL == pending) {
        goto _unref_message_err;
    }
    dbus_connection_flush(conn);

    // block until we receive a reply
    dbus_pending_call_block(pending);

    DBusMessage* reply = NULL;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);
    if (NULL == reply) {
        goto _unref_pending_err;
    }

    dbus_message_unref(reply);
_unref_pending_err:
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "error: %s\n", err.message);
        dbus_error_free(&err);
        status = -1;
    }

    // free the pending message handle
    dbus_pending_call_unref(pending);
_unref_message_err:
    // free message
    dbus_message_unref(msg);

    return status;
}

int load_mpris_players(DBusConnection* conn, mpris_player *players)
{
    if (NULL == conn) { return 0; }
    if (NULL == players) { return 0; }

    char* method = DBUS_METHOD_LIST_NAMES;
    char* destination = DBUS_DESTINATION;
    char* path = DBUS_PATH;
    char* interface = DBUS_INTERFACE;

    DBusMessage* msg;
    DBusPendingCall* pending;
    int cnt = 0;

    DBusError err = {0};
    dbus_error_init(&err);

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return cnt; }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT) ||
        NULL == pending) {
        goto _unref_message_err;
    }
    dbus_connection_flush(conn);

    // block until we receive a reply
    dbus_pending_call_block(pending);

    DBusMessage* reply = NULL;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);
    if (NULL == reply) { goto _unref_pending_err; }

    DBusMessageIter rootIter;
    if (dbus_message_iter_init(reply, &rootIter) && DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&rootIter)) {
        DBusMessageIter arrayElementIter;

        dbus_message_iter_recurse(&rootIter, &arrayElementIter);
        while (cnt < MAX_PLAYERS) {
            if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&arrayElementIter)) {
                char *str = NULL;
                dbus_message_iter_get_basic(&arrayElementIter, &str);
                if (!strncmp(str, MPRIS_PLAYER_NAMESPACE, strlen(MPRIS_PLAYER_NAMESPACE))) {
                    memcpy(players[cnt].namespace, str, strlen(str)+1);
                    cnt++;
                }
            }
            if (!dbus_message_iter_has_next(&arrayElementIter)) {
                break;
            }
            dbus_message_iter_next(&arrayElementIter);
        }
    }
    dbus_message_unref(reply);

_unref_pending_err:
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "error: %s\n", err.message);
        dbus_error_free(&err);
    }

    // free the pending message handle
    dbus_pending_call_unref(pending);
_unref_message_err:
    // free message
    dbus_message_unref(msg);
    return cnt;
}
