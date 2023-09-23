/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "sstring.h"
#include "sdbus.h"

#define CMD_HELP        "help"
#define CMD_PLAY        "play"
#define CMD_PAUSE       "pause"
#define CMD_STOP        "stop"
#define CMD_NEXT        "next"
#define CMD_PREVIOUS    "prev"
#define CMD_PLAY_PAUSE  "pp"
#define CMD_STATUS      "status"
#define CMD_SEEK        "seek"

#define CMD_LIST        "list"
#define CMD_INFO        "info"
#define ARG_PLAYER      "--player"

#define INFO_DEFAULT_STATUS "%track_name - %album_name - %artist_name"
#define INFO_FULL_STATUS    "Player name:\t" INFO_PLAYER_NAME "\n" \
"Play status:\t" INFO_PLAYBACK_STATUS "\n" \
"Track:\t\t" INFO_TRACK_NAME "\n" \
"Artist:\t\t" INFO_ARTIST_NAME "\n" \
"Album:\t\t" INFO_ALBUM_NAME "\n" \
"Album Artist:\t" INFO_ALBUM_ARTIST "\n" \
"Art URL:\t" INFO_ART_URL "\n" \
"Track:\t\t" INFO_TRACK_NUMBER "\n" \
"Length:\t\t" INFO_TRACK_LENGTH "\n" \
"Volume:\t\t" INFO_VOLUME "\n" \
"Loop status:\t" INFO_LOOP_STATUS "\n" \
"Shuffle:\t" INFO_SHUFFLE_MODE "\n" \
"Position:\t" INFO_POSITION "\n" \
"Bitrate:\t" INFO_BITRATE "\n" \
"Comment:\t" INFO_COMMENT \
""

#define INFO_PLAYER_NAME     "%player_name"
#define INFO_TRACK_NAME      "%track_name"
#define INFO_TRACK_NUMBER    "%track_number"
#define INFO_TRACK_LENGTH    "%track_length"
#define INFO_ARTIST_NAME     "%artist_name"
#define INFO_ALBUM_NAME      "%album_name"
#define INFO_ALBUM_ARTIST    "%album_artist"
#define INFO_ART_URL         "%art_url"
#define INFO_BITRATE         "%bitrate"
#define INFO_COMMENT         "%comment"

#define INFO_PLAYBACK_STATUS "%play_status"
#define INFO_SHUFFLE_MODE    "%shuffle"
#define INFO_VOLUME          "%volume"
#define INFO_LOOP_STATUS     "%loop_status"
#define INFO_POSITION        "%position"

#define INFO_FULL            "%full"

#define TRUE_LABEL      "true"
#define FALSE_LABEL     "false"

#define PLAYER_ACTIVE    "active"
#define PLAYER_INACTIVE  "inactive"

#define HELP_MESSAGE    "MPRIS control, version %s\n" \
"Usage:\n  %s [" ARG_PLAYER " " PLAYER_ACTIVE " | " PLAYER_INACTIVE " | <name ...>] COMMAND - Control running MPRIS player\n" \
"\n" \
"Options:\n" \
ARG_PLAYER" "PLAYER_ACTIVE"\t\tExecute command only for the active player(s) (default)\n" \
"         "PLAYER_INACTIVE"\tExecute command only for the inactive player(s)\n" \
"         <name ...>\tExecute command only for player(s) named <name ...>\n" \
"\n" \
"Commands:\n"\
"\t" CMD_HELP "\t\tThis help message\n" \
"\n" \
"\t" CMD_PLAY "\t\tBegin playing\n" \
"\t" CMD_PLAY_PAUSE "\t\tToggle play or pause\n" \
"\t" CMD_PAUSE "\t\tPause the player\n" \
"\t" CMD_STOP "\t\tStop the player\n" \
"\t" CMD_NEXT "\t\tChange track to the next in the playlist\n" \
"\t" CMD_PREVIOUS "\t\tChange track to the previous in the playlist\n" \
"\t" CMD_SEEK "\t\t[time[ms|s|m] Seek forwards or backwards in current track for 'time'.\n" \
"\t\t\tThe time can be a float value, if absent it defaults to 10 seconds.\n" \
"\t\t\tThe unit can be one of ms(milliseconds), s(seconds), m(minutes), if absent it defaults to seconds.\n" \
"\n" \
"\t" CMD_INFO "\t\t<format> Display information about the current track.\n" \
"\t\t\tThe default format is '%s'\n" \
"\t" CMD_STATUS "\t\tGet the playback status (equivalent to '" CMD_INFO " %" INFO_PLAYBACK_STATUS "')\n" \
"\t" CMD_LIST "\t\tGet the name of the running player(s) (equivalent to '" CMD_INFO " %" INFO_PLAYER_NAME "')\n" \
"\n" \
"Format specifiers for " CMD_INFO " command:\n" \
"\t%" INFO_PLAYER_NAME "\tprints the player name\n" \
"\t%" INFO_TRACK_NAME "\tprints the track name\n" \
"\t%" INFO_TRACK_NUMBER "\tprints the track number\n" \
"\t%" INFO_TRACK_LENGTH "\tprints the track length in seconds\n" \
"\t%" INFO_ARTIST_NAME "\tprints the artist name\n" \
"\t%" INFO_ALBUM_NAME "\tprints the album name\n" \
"\t%" INFO_ALBUM_ARTIST "\tprints the album artist\n" \
"\t%" INFO_ART_URL "\tprints the URL of the cover art image\n" \
"\t%" INFO_PLAYBACK_STATUS "\tprints the playback status\n" \
"\t%" INFO_SHUFFLE_MODE "\tprints the shuffle mode\n" \
"\t%" INFO_VOLUME "\t\tprints the volume\n" \
"\t%" INFO_LOOP_STATUS "\tprints the loop status\n" \
"\t%" INFO_POSITION "\tprints the song position in seconds\n" \
"\t%" INFO_BITRATE "\tprints the track's bitrate\n" \
"\t%" INFO_COMMENT "\tprints the track's comment\n" \
"\t%" INFO_FULL "\t\tprints all available information\n" \
""

const char* get_version(void)
{
#ifndef VERSION_HASH
#define VERSION_HASH "(unknown)"
#endif
    return VERSION_HASH;
}

enum cmd {
    c_help,
    c_play,
    c_pause,
    c_stop,
    c_next,
    c_previous,
    c_play_pause,
    c_status,
    c_seek,
    c_list,
    c_info,
    c_count
};

struct ctl {
    int status;
    enum cmd command;

    bool show_active_players;
    bool show_inactive_players;

    char player_names[MAX_PLAYERS][MAX_OUTPUT_LENGTH];
    mpris_player players[MAX_PLAYERS];
    int player_count;

};

const char* get_dbus_property_name (enum cmd command)
{
    if (command == c_status) {
        return MPRIS_PROP_PLAYBACK_STATUS;
    }
    if (command == c_info) {
        return MPRIS_PROP_METADATA;
    }
    if (command == c_list) {
        return INFO_PLAYER_NAME;
    }

    return NULL;
}

const char* get_dbus_method (enum cmd command)
{
    if (command == c_play) {
        return MPRIS_METHOD_PLAY;
    }
    if (command == c_pause) {
        return MPRIS_METHOD_PAUSE;
    }
    if (command == c_stop) {
        return MPRIS_METHOD_STOP;
    }
    if (command == c_next) {
        return MPRIS_METHOD_NEXT;
    }
    if (command == c_previous) {
        return MPRIS_METHOD_PREVIOUS;
    }
    if (command == c_play_pause) {
        return MPRIS_METHOD_PLAY_PAUSE;
    }
    if (command == c_seek) {
        return MPRIS_METHOD_SEEK;
    }
    if (command == c_status || command == c_info || command == c_list) {
        return DBUS_PROPERTIES_INTERFACE;
    }

    return NULL;
}

void print_help(char* name)
{
    const char* help_msg;
    const char* version = get_version();

    help_msg = HELP_MESSAGE;
    char* info_def = INFO_DEFAULT_STATUS;

    fprintf(stdout, help_msg, version, name, info_def);
}

void print_mpris_info(mpris_properties *props, const char* format)
{
    const char* info_full = INFO_FULL_STATUS;
    const char* shuffle_label = (props->shuffle ? TRUE_LABEL : FALSE_LABEL);
    char volume_label[5];
    snprintf(volume_label, 5, "%.2f", props->volume);
    char pos_label[11];
    snprintf(pos_label, 11, "%.2lfs", (props->position / 1000000.0));
    char track_number_label[6];
    snprintf(track_number_label, 6, "%d", props->metadata.track_number);
    char bitrate_label[6];
    snprintf(bitrate_label, 6, "%d", props->metadata.bitrate);
    char length_label[15];
    snprintf(length_label, 15, "%.2lfs", (props->metadata.length / 1000000.0));

    char output[MAX_OUTPUT_LENGTH*10];
    memcpy(output, format, strlen(format) + 1);

    str_replace(output, "\\n", "\n");
    str_replace(output, "\\t", "\t");

    str_replace(output, INFO_FULL, info_full);
    str_replace(output, INFO_PLAYER_NAME, props->player_name);
    str_replace(output, INFO_SHUFFLE_MODE, shuffle_label);
    str_replace(output, INFO_PLAYBACK_STATUS, props->playback_status);
    str_replace(output, INFO_VOLUME, volume_label);
    str_replace(output, INFO_LOOP_STATUS, props->loop_status);
    str_replace(output, INFO_POSITION, pos_label);
    str_replace(output, INFO_TRACK_NAME, props->metadata.title);
    str_replace(output, INFO_ARTIST_NAME, props->metadata.artist);
    str_replace(output, INFO_ALBUM_ARTIST, props->metadata.album_artist);
    str_replace(output, INFO_ALBUM_NAME, props->metadata.album);
    str_replace(output, INFO_TRACK_LENGTH, length_label);
    str_replace(output, INFO_TRACK_NUMBER, track_number_label);
    str_replace(output, INFO_BITRATE, bitrate_label);
    str_replace(output, INFO_COMMENT, props->metadata.comment);
    str_replace(output, INFO_ART_URL, props->metadata.art_url);

    fprintf(stdout, "%s\n", output);
}

#define DEFAULT_SKEEP_MSEC       5*1000 // 5 seconds

#define TIME_SUFFIX_SEC          "s"
#define TIME_SUFFIX_MIN          "m"
#define TIME_SUFFIX_MSEC         "ms"

int parse_time_argument(char *time_string)
{
    int ms = DEFAULT_SKEEP_MSEC;
    if (NULL == time_string) { return ms; }

    float time_units = -1.0;
    char suffix[10] = {0};

    int loaded = sscanf(time_string, "%f%s", &time_units, (char*)&suffix);
    if (loaded == 0 || loaded == EOF) return ms;
    if (loaded == 1) suffix[0] = 's';

    if (strncmp(suffix, TIME_SUFFIX_SEC, 1) == 0) {
        ms = time_units * 1000;
    }
    if (strncmp(suffix, TIME_SUFFIX_MIN, 1) == 0) {
        ms = time_units * 60 * 1000;
    }
    if (strncmp(suffix, TIME_SUFFIX_MSEC, 2) == 0) {
        ms = time_units;
    }

    return ms;
}

bool arg_is_command(const char *param)
{
    if (NULL == param) return false;

    const char commands[11][9] = {CMD_HELP, CMD_PLAY, CMD_PAUSE, CMD_STOP, CMD_NEXT,
        CMD_PREVIOUS, CMD_PLAY_PAUSE, CMD_STATUS, CMD_SEEK, CMD_LIST, CMD_INFO};

    for (int i = 0; i < (int)array_size(commands); i++) {
        const char *cmd = commands[i];
        if (strncmp(param, cmd, strlen(cmd)) == 0) {
            return true;
        }
    }
    return false;
}

void load_players(struct ctl *cmd, DBusConnection *conn, char *params[], int param_count)
{
    static struct option long_options[] = {
        {"player", required_argument, NULL, 1},
        {"help", no_argument, NULL, 2},
        {0},
    };

    int player_names_count = 0;
    opterr = 0; // Skip errors
    while (true) {
        int char_arg = getopt_long(param_count, params, "", long_options, NULL);
        if (char_arg == -1) { break; }
        switch (char_arg) {
            case 1:
                if (strncmp(optarg, PLAYER_ACTIVE, strlen(PLAYER_ACTIVE)) == 0) {
                    cmd->show_active_players = true;
                    continue;
                }
                if (strncmp(optarg, PLAYER_INACTIVE, strlen(PLAYER_INACTIVE)) == 0) {
                    cmd->show_inactive_players = true;
                    continue;
                }
                optind--;
                for( ;optind < param_count && *params[optind] != '-'; optind++){
                    optarg = params[optind];
                    int len = strlen(optarg);
                    memcpy(cmd->player_names[player_names_count++], optarg, MIN(MAX_OUTPUT_LENGTH - 1, len));
                }
                break;
            case 2:
                // TODO(marius): I should make it that the --help argument shows the current command's help
                // Currently we just show full help.
                cmd->command = c_help;
                break;
            default:
                break;
        }
    }

    if (!cmd->show_active_players && !cmd->show_inactive_players && player_names_count == 0) {
        cmd->show_active_players = false;
        cmd->show_inactive_players = false;
    }

    cmd->player_count = load_mpris_players(conn, cmd->players);
    for (int i = 0; i < cmd->player_count; i++) {
        mpris_player *player = &cmd->players[i];
        load_mpris_properties(conn, player->namespace, &player->properties);

        player->skip = true;
        if (cmd->show_active_players && (strncmp(player->properties.playback_status, MPRIS_METADATA_VALUE_PLAYING, 8) == 0)) {
            player->skip = false;
        }

        if (cmd->show_inactive_players &&
            (strncmp(player->properties.playback_status, MPRIS_METADATA_VALUE_PAUSED, 7) == 0 ||
            strncmp(player->properties.playback_status, MPRIS_METADATA_VALUE_STOPPED, 8) == 0)) {
            player->skip = false;
        }

        for (int i = 0; i < player_names_count; i++) {
            char *player_name = cmd->player_names[i];
            if (NULL != player_name) {
                size_t name_len = strlen(player_name);
                size_t prop_name_len = strlen(player->properties.player_name);
                size_t prop_ns_len = strlen(player->namespace);
                if (prop_name_len < name_len) {
                    prop_name_len = name_len ;
                }
                if (prop_ns_len < name_len) {
                    prop_ns_len = name_len;
                }
                if (strncmp(player->properties.player_name, player_name, prop_name_len) == 0 ||
                   strncmp(player->namespace, player_name, prop_ns_len) == 0) {
                    player->skip = false;
                }
            }
        }
    }
}

int main(int argc, char** argv)
{
    struct ctl cmd = {0};
    cmd.status = EXIT_FAILURE;

    char* name = argv[0];
    if (argc == 0) {
        cmd.command = c_help;
        goto _help;
    }
    int ms = DEFAULT_SKEEP_MSEC;
    char **params = calloc(argc+1, sizeof(char*));
    int param_count = 0;

    /**
     * First we go through the arguments to determine the command
     * For particular commands we get the parameters that are relevant to it:
     *  * "seek" needs the amount of time units to skeep ahead or behind (if negative)
     *  * "info" needs the string format
     * The remaining arguments are parsed with the getopt_* API.
     * Currently we only have the --help and --player arguments. The former can take multiple
     * MPRIS namespaces, or player names, together with the "active"/"inactive" special values.
     */
    char *info_format = NULL;
    for (int i = 1; i < argc; i++) {
        if (arg_is_command(argv[i])) {
            char *command = argv[i];
            if (strncmp(command, CMD_SEEK, strlen(CMD_SEEK)) == 0) {
                cmd.command = c_seek;
                if (i <= argc) {
                    ms = parse_time_argument(argv[++i]);
                }
            } else if (strncmp(command, CMD_INFO, strlen(CMD_INFO)) == 0 && argc > i+1) {
                cmd.command = c_info;
                if (*argv[i+1] != '-') info_format = argv[++i];
            } else if (strncmp(command, CMD_STATUS, strlen(CMD_STATUS)) == 0) {
                cmd.command = c_status;
                info_format = INFO_PLAYBACK_STATUS;
            } else if (strncmp(command, CMD_LIST, strlen(CMD_LIST)) == 0) {
                cmd.command = c_list;
                info_format = INFO_PLAYER_NAME;
            }
        } else {
            params[param_count++] = argv[i];
        }
    }
    if (cmd.command == c_info && NULL == info_format) {
        info_format = INFO_DEFAULT_STATUS;
    }

    if (cmd.command == c_help) {
        goto _help;
    }

    // initialise the errors
    DBusError err = {0};
    dbus_error_init(&err);

    // connect to the system bus and check for errors
    DBusConnection *conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "DBus connection error(%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (NULL == conn) {
        goto _exit;
    }
    load_players(&cmd, conn, argv, argc);

    char *dbus_method = (char*)get_dbus_method(cmd.command);
    if (NULL == dbus_method) {
        //fprintf(stderr, "Invalid command %s (use help for help)\n", command);
        goto _exit;
    }
    if (cmd.player_count == 0) {
        fprintf(stderr, "No players found\n");
        goto _exit;
    }

    for (int i = 0; i < cmd.player_count; i++) {
        mpris_player player = cmd.players[i];
        if (player.skip) continue;

        const char *dbus_property = (char*)get_dbus_property_name(cmd.command);
        if (NULL == dbus_property) {
            if (cmd.command == c_seek) {
                seek (conn, player, ms);
            } else {
                call_dbus_method(conn, player.namespace, MPRIS_PLAYER_PATH, MPRIS_PLAYER_INTERFACE, dbus_method);
            }
        } else {
            print_mpris_info(&player.properties, info_format);
        }
    }
    cmd.status = EXIT_SUCCESS;

    if (NULL != conn) {
        dbus_connection_close(conn);
        dbus_connection_unref(conn);
    }
_exit:
    return cmd.status;
_help:
    print_help(name);
    goto _exit;
}

