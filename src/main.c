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
#define CMD_SHUFFLE     "shuffle"
#define CMD_REPEAT      "repeat"
#define CMD_VOLUME      "volume"

#define CMD_LIST        "list"
#define CMD_INFO        "info"

#define ARG_PLAYER       "--player"
#define ARG_REPEAT_TRACK "--track"
#define ARG_REPEAT_PLIST "--playlist"

#define PLAYER_ACTIVE    "active"
#define PLAYER_INACTIVE  "inactive"

#define BOOL_ON          "on"
#define BOOL_OFF         "off"

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

#define MAX_VOLUME      100.0f
#define MIN_VOLUME        0.0f

#define HELP_MESSAGE    "MPRIS control, version %s\n" \
"Usage:\n  %s [" ARG_PLAYER " " PLAYER_ACTIVE " | " PLAYER_INACTIVE " | <name ...>] [COMMAND] - Control running MPRIS player\n" \
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
"\n" \
"\t" CMD_SHUFFLE "\t\t[" BOOL_ON "|" BOOL_OFF "] Change shuffle mode to on or off. If argument is absent it toggles the mode.\n" \
"\t" CMD_REPEAT "\t\t[" ARG_REPEAT_TRACK "|" ARG_REPEAT_PLIST "][" BOOL_ON "|" BOOL_OFF "] Change the loop status. If argument is absent it toggles the mode.\n" \
"\n" \
"\t" CMD_SEEK "\t\t[-][time][ms|s|m] Seek forwards or backwards in current track for 'time'.\n" \
"\t\t\tThe time can be a float value, if absent it defaults to 5 seconds.\n" \
"\t\t\tThe time can be a negative value, which seeks backwards.\n" \
"\t\t\tThe unit can be one of ms(milliseconds), s(seconds), m(minutes), if absent it defaults to seconds.\n" \
"\n" \
"\t" CMD_VOLUME "\t\t[+|-][percentage] Modify the volume for the player.\n" \
"\t\t\tThe percentage can be a float value from 0 to 100 to set the volume to the respective percentage level.\n" \
"\t\t\tThe percentage can be prefixed by a \"+\", or a \"-\", to increase, or decrease the volume by the respective value.\n" \
"\t\t\tIf the percentage argument is not present, the current volume will be output (equivalent to '" CMD_INFO " %" INFO_VOLUME "').\n" \
"\n" \
"\t" CMD_STATUS "\t\tGet the playback status (equivalent to '" CMD_INFO " %" INFO_PLAYBACK_STATUS "')\n" \
"\t" CMD_LIST "\t\tGet the name of the running player(s) (equivalent to '" CMD_INFO " %" INFO_PLAYER_NAME "')\n" \
"\t" CMD_INFO "\t\t<format> Display information about the current track.\n" \
"\t\t\tThe default format is '%s'\n" \
"\n" \
"Format specifiers for " CMD_INFO " command:\n" \
"\t%" INFO_PLAYER_NAME "\tprints the player name\n" \
"\t%" INFO_TRACK_NAME "\tprints the track name\n" \
"\t%" INFO_TRACK_NUMBER "\tprints the track number\n" \
"\t%" INFO_TRACK_LENGTH "\tprints the track length\n" \
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

const char commands[14][9] = {CMD_HELP, CMD_PLAY, CMD_PAUSE, CMD_STOP, CMD_NEXT, CMD_PREVIOUS,
    CMD_PLAY_PAUSE, CMD_STATUS, CMD_SEEK, CMD_LIST, CMD_INFO, CMD_SHUFFLE, CMD_REPEAT, CMD_VOLUME, };

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
    c_shuffle,
    c_repeat,
    c_volume,

    c_count
};

struct ctl {
    int status;
    enum cmd command;

    char player_names[MAX_PLAYERS][MAX_OUTPUT_LENGTH];
    mpris_player players[MAX_PLAYERS];
    int player_count;

};

int volume_change_valid(const struct volume_change v)
{
    if (v.type == volume_change_absolute) {
        if (v.value < MIN_VOLUME) return -1;
    }
    if (v.value > MAX_VOLUME) return -1;

    return 0;
}
const char *get_dbus_method (enum cmd command)
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
    if (command == c_shuffle || command == c_repeat || command == c_volume) {
        return DBUS_METHOD_SET;
    }
    if (command == c_status || command == c_info || command == c_list) {
        return DBUS_INTERFACE_PROPERTIES;
    }

    return NULL;
}

void print_help(char* name)
{
    const char* version = get_version();

    const char *help_msg = HELP_MESSAGE;
    char* info_def = INFO_DEFAULT_STATUS;

    fprintf(stdout, help_msg, version, name, info_def);
}

void format_nanosecond_interval(char *destination, const size_t max_len, const int64_t time_nanoseconds)
{
    int32_t time_seconds = time_nanoseconds / 1000000;
    const short time_minutes = time_seconds / 60;
    const short time_hours = time_minutes / 60;
    const short time_days = time_hours / 24;
    if (time_minutes > 0) {
        if (time_hours > 0) {
            if (time_days > 0) {
                snprintf(destination, max_len, "%dd %dh %dm %ds", time_days, time_hours, time_minutes, time_seconds % 60);
                return;
            }
            snprintf(destination, max_len, "%dh %dm %ds", time_hours, time_minutes, time_seconds % 60);
            return;
        }
        snprintf(destination, max_len, "%dm %ds", time_minutes, time_seconds % 60);
        return;
    }

    snprintf(destination, max_len, "%ds", time_seconds);
}

void print_mpris_info(const mpris_properties *props, const char* format)
{
    const char* info_full = INFO_FULL_STATUS;
    const char* shuffle_label = (props->shuffle ? TRUE_LABEL : FALSE_LABEL);
    char volume_label[8];
    snprintf(volume_label, 8, "%.2lf%%", props->volume*MAX_VOLUME);
    char pos_label[20];
    format_nanosecond_interval(pos_label, 20, props->position);
    char track_number_label[6];
    snprintf(track_number_label, 6, "%d", props->metadata.track_number);
    char bitrate_label[6];
    snprintf(bitrate_label, 6, "%d", props->metadata.bitrate);
    char length_label[20];
    format_nanosecond_interval(length_label, 20, props->metadata.length);

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

enum bool_arg {
    b_unset,
    b_on,
    b_off,
};

enum repeat_mode {
    ls_track,
    ls_playlist,
    ls_count,
};

int parse_bool_argument(const char *bool_string, enum bool_arg *state)
{
    if (strncmp(bool_string, FALSE_LABEL, strlen(FALSE_LABEL)) == 0) {
        *state = b_off;
    } else if (strncmp(bool_string, TRUE_LABEL, strlen(TRUE_LABEL)) == 0) {
        *state = b_on;
    } else if (strncmp(bool_string, BOOL_OFF, strlen(BOOL_OFF)) == 0) {
        *state = b_off;
    } else if (strncmp(bool_string, BOOL_ON, strlen(BOOL_ON)) == 0) {
        *state = b_on;
    } else if (strncmp(bool_string, "1", 1) == 0) {
        *state = b_on;
    } else if (strncmp(bool_string, "0", 1) == 0) {
        *state = b_off;
    } else {
        return -1;
    }
    return 0;
}

int parse_time_argument(const char *time_string, int *ms)
{
    *ms = DEFAULT_SKEEP_MSEC;
    if (NULL == time_string) { return 0; }

    float time_units = -1.0;
    char suffix[10] = {0};

    const int loaded = sscanf(time_string, "%f%s", &time_units, (char*)&suffix);
    if (loaded == 0 || loaded == EOF) return 0;
    if (loaded == 1) suffix[0] = 's';

    if (strncmp(suffix, TIME_SUFFIX_SEC, 1) == 0) {
        *ms = time_units * 1000;
    }
    if (strncmp(suffix, TIME_SUFFIX_MIN, 1) == 0) {
        *ms = time_units * 60 * 1000;
    }
    if (strncmp(suffix, TIME_SUFFIX_MSEC, 2) == 0) {
        *ms = time_units;
    }

    return 0;
}

int parse_volume_argument(const char *decimal_string, struct volume_change *vol)
{
    char suffix = decimal_string[0];
    char *value = (char*)decimal_string;
    if (decimal_string[0] == '+' || decimal_string[0] == '-') {
        vol->type = volume_change_relative;
        value = (char*)decimal_string+1;
    }

    const int loaded = sscanf(value, "%lf", &vol->value);
    if (loaded == 0) {
        return -1;
    }
    if (suffix == '-') {
        vol->value = -1.0l * vol->value;
    }
    return 0;
}

bool arg_is_command(const char *param)
{
    if (NULL == param) return false;

    for (int i = 0; i < (int)array_size(commands); i++) {
        const char *cmd = commands[i];
        if (strncmp(param, cmd, strlen(cmd)) == 0) {
            return true;
        }
    }
    return false;
}

void load_players_flags(struct ctl *cmd, DBusConnection *conn, char *params[], int param_count, enum repeat_mode *ls)
{
    static struct option long_options[] = {
        {"player", required_argument, NULL, 1},
        {"help", no_argument, NULL, 2},
        {"track", no_argument, NULL, 3},
        {"playlist", no_argument, NULL, 4},
        {0},
    };

    opterr = 0; // Skip errors

    int player_names_count = 0;
    bool active_players = false;
    bool inactive_players = false;
    while (true) {
        const int char_arg = getopt_long(param_count, params, "", long_options, NULL);
        if (char_arg == -1) { break; }
        switch (char_arg) {
            case 1:
                if (strncmp(optarg, PLAYER_ACTIVE, strlen(PLAYER_ACTIVE)) == 0) {
                    active_players = true;
                    continue;
                }
                if (strncmp(optarg, PLAYER_INACTIVE, strlen(PLAYER_INACTIVE)) == 0) {
                    inactive_players = true;
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
            case 3:
                *ls = ls_track;
                break;
            case 4:
                *ls = ls_playlist;
                break;
            default:
                break;
        }
    }

    if (!active_players && !inactive_players && player_names_count == 0) {
        active_players = true;
        inactive_players = false;
    }

    cmd->player_count = load_mpris_players(conn, cmd->players);
    for (int i = 0; i < cmd->player_count; i++) {
        mpris_player *player = &cmd->players[i];
        load_mpris_properties(conn, player->namespace, &player->properties);

        player->skip = true;
        if (active_players && (strncmp(player->properties.playback_status, MPRIS_METADATA_VALUE_PLAYING, 8) == 0)) {
            player->skip = false;
        }

        if (inactive_players &&
            (strncmp(player->properties.playback_status, MPRIS_METADATA_VALUE_PAUSED, 7) == 0 ||
            strncmp(player->properties.playback_status, MPRIS_METADATA_VALUE_STOPPED, 8) == 0)) {
            player->skip = false;
        }

        for (int i = 0; i < player_names_count; i++) {
            const char *player_name = cmd->player_names[i];
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

bool has_next_argument(const int argc, char** argv, const int i)
{
    return i+1 < argc && strncmp(argv[i+1], "--", 2) != 0;
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
    char *info_format = NULL;

    bool shuffle_mode = false;
    enum bool_arg on_arg;
    enum repeat_mode repeat_mode = {0};
    struct volume_change volume = {0};

    /**
     * First we go through the arguments to determine the command
     * For particular commands we get the parameters that are relevant to it:
     *  * "seek" needs the amount of time units to skeep ahead or behind (if negative)
     *  * "info" needs the string format
     * The remaining arguments are parsed with the getopt_* API.
     * Currently we only have the --help and --player arguments. The former can take multiple
     * MPRIS namespaces, or player names, together with the "active"/"inactive" special values.
     */
    for (int i = 1; i < argc; i++) {
        if (arg_is_command(argv[i])) {
            char *command = argv[i];
            if (strncmp(command, CMD_SEEK, strlen(CMD_SEEK)) == 0) {
                cmd.command = c_seek;
                if (has_next_argument(argc, argv, i)) {
                    parse_time_argument(argv[++i], &ms);
                }
            } else if (strncmp(command, CMD_INFO, strlen(CMD_INFO)) == 0) {
                cmd.command = c_info;
                if (has_next_argument(argc, argv, i)) {
                    info_format = argv[i+1];
                    i++;
                }
            } else if (strncmp(command, CMD_STATUS, strlen(CMD_STATUS)) == 0) {
                cmd.command = c_status;
                info_format = INFO_PLAYBACK_STATUS;
            } else if (strncmp(command, CMD_LIST, strlen(CMD_LIST)) == 0) {
                cmd.command = c_list;
                info_format = INFO_PLAYER_NAME;
            } else if (strncmp(command, CMD_PLAY_PAUSE, strlen(CMD_PLAY_PAUSE)) == 0) {
                cmd.command = c_play_pause;
            } else if (strncmp(command, CMD_PLAY, strlen(CMD_PLAY)) == 0) {
                cmd.command = c_play;
            } else if (strncmp(command, CMD_PAUSE, strlen(CMD_PAUSE)) == 0) {
                cmd.command = c_pause;
            } else if (strncmp(command, CMD_PREVIOUS, strlen(CMD_PREVIOUS)) == 0) {
                cmd.command = c_previous;
            } else if (strncmp(command, CMD_NEXT, strlen(CMD_NEXT)) == 0) {
                cmd.command = c_next;
            } else if (strncmp(command, CMD_STOP, strlen(CMD_STOP)) == 0) {
                cmd.command = c_stop;
            } else if (strncmp(command, CMD_SHUFFLE, strlen(CMD_SHUFFLE)) == 0) {
                cmd.command = c_shuffle;
                if (has_next_argument(argc, argv, i)) {
                    char *state = argv[++i];
                    if (parse_bool_argument(state, &on_arg) < 0) {
                        fprintf(stderr, "Invalid shuffle argument '%s'. Use one of '" BOOL_ON "'/'" BOOL_OFF "'.\n", state);
                        goto _exit;
                    }
                }
            } else if (strncmp(command, CMD_REPEAT, strlen(CMD_REPEAT)) == 0) {
                cmd.command = c_repeat;
                if (has_next_argument(argc, argv, i)) {
                    char *state = argv[++i];
                    if (parse_bool_argument(state, &on_arg) < 0) {
                        fprintf(stderr, "Invalid repeat argument '%s'. Use one of '" BOOL_ON "'/'" BOOL_OFF "'.\n", state);
                        goto _exit;
                    }
                }
            } else if (strncmp(command, CMD_VOLUME, strlen(CMD_VOLUME)) == 0) {
                cmd.command = c_volume;
                if (!has_next_argument(argc, argv, i)) {
                    cmd.command = c_info;
                    info_format = INFO_VOLUME;
                    break;
                }
                char *state = argv[++i];
                if (parse_volume_argument(state, &volume) < 0) {
                    fprintf(stderr, "Invalid volume argument '%s'. Use a float value.\n", state);
                    goto _exit;
                }
                if (volume_change_valid(volume) < 0) {
                    if (volume.type == volume_change_absolute) {
                        fprintf(stderr, "Invalid volume value '%s'. Use a value between %.2f%% andd %.2f%%.\n", state, MIN_VOLUME, MAX_VOLUME);
                    } else {
                        fprintf(stderr, "Invalid volume value '%s'. Use a value less than %.2f%%.\n", state, MAX_VOLUME);
                    }
                    goto _exit;
                }
            }
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
    load_players_flags(&cmd, conn, argv, argc, &repeat_mode);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "error: %s\n", err.message);
        dbus_error_free(&err);
    }

    char *dbus_method = (char*)get_dbus_method(cmd.command);
    if (NULL == dbus_method) {
        goto _exit;
    }
    if (cmd.player_count == 0) {
        fprintf(stderr, "No players found.\n");
        goto _exit;
    }

    for (int i = 0; i < cmd.player_count; i++) {
        mpris_player player = cmd.players[i];
        if (player.skip) {
            if (cmd.player_names[0][0] != 0) continue;
            if (cmd.command != c_play) continue;
        }

        if (cmd.command == c_info || cmd.command == c_status || cmd.command == c_list) {
            print_mpris_info(&player.properties, info_format);
            cmd.status = EXIT_SUCCESS;
        } else if (cmd.command == c_seek) {
            if (seek(conn, player, ms) > 0) {
                cmd.status = EXIT_SUCCESS;
            }
        } else if (cmd.command == c_shuffle) {
            if (on_arg == b_unset) {
                shuffle_mode = !player.properties.shuffle;
            } else if (on_arg == b_on) {
                shuffle_mode = true;
            } else if (on_arg == b_off) {
                shuffle_mode = false;
            }
            if (shuffle(conn, player, &shuffle_mode) > 0) {
                cmd.status = EXIT_SUCCESS;
            }
        } else if (cmd.command == c_repeat) {
            const char* state;
            if (on_arg == b_on) {
                if (repeat_mode == ls_track) {
                    state = MPRIS_LOOPSTATUS_VALUE_TRACK;
                } else {
                    state = MPRIS_LOOPSTATUS_VALUE_PLAYLIST;
                }
            } else if (on_arg == b_off) {
                state = MPRIS_LOOPSTATUS_VALUE_NONE;
            } else {
                if (strncmp(player.properties.loop_status, MPRIS_LOOPSTATUS_VALUE_NONE, strlen(MPRIS_LOOPSTATUS_VALUE_NONE)) != 0) {
                    state = MPRIS_LOOPSTATUS_VALUE_NONE;
                } else {
                    if (repeat_mode == ls_track) {
                        state = MPRIS_LOOPSTATUS_VALUE_TRACK;
                    } else {
                        state = MPRIS_LOOPSTATUS_VALUE_PLAYLIST;
                    }
                }
            }
            if (set_loopstatus(conn, player, state) > 0) {
                cmd.status = EXIT_SUCCESS;
            }
        } else if (cmd.command == c_volume) {
            double abs_volume = volume.value / MAX_VOLUME;
            if (volume.type == volume_change_relative) {
                abs_volume += player.properties.volume;
            }
            if (set_volume(conn, player, abs_volume) > 0) {
                cmd.status = EXIT_SUCCESS;
            }
        } else {
            DBusMessage *msg = call_dbus_method(conn, player.namespace, MPRIS_PLAYER_PATH, MPRIS_PLAYER_INTERFACE, dbus_method);
            if (NULL != msg) {
                cmd.status = EXIT_SUCCESS;
            }
        }
        if (dbus_error_is_set(&err)) {
            fprintf(stderr, "error: %s\n", err.message);
            dbus_error_free(&err);
        }
    }

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

