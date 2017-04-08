/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <string.h>
#include <stdio.h>

//#define MAX_OUTPUT_LENGTH 1048576
#define MAX_OUTPUT_LENGTH 1024

char* get_zero_string(size_t length)
{
    return (char*)calloc(1, sizeof(char) * (length + 1));
}

char* str_replace(char* source, const char* search, const char* replace)
{
    if (NULL == source) { return source; }
    size_t so_len = strlen(source);
    fprintf(stderr, "%p\n", source);

    if (so_len == 0) { return 0; }

    if (NULL == search) { return source; }
    if (NULL == replace) { return source; }

    size_t se_len = strlen(search);
    if (se_len == 0) { return source; }

    size_t re_len = strlen(replace);

    if (se_len > so_len) { return source; }

    size_t max_matches = so_len / se_len;

    size_t matches[max_matches];
    size_t matches_cnt = 0;
    // get the number of matches for replace in source
    for (size_t so_i = 0; so_i < so_len; so_i++) {
        size_t se_i = 0;
        size_t matched = 0;
        while (se_i < se_len) {
            if (search[se_i] == (source)[so_i + se_i]) {
                matched++;
            } else {
                break;
            }
            if (matched == se_len) {
                // search was found in source
                matches[matches_cnt++] = so_i;
                so_i += se_i;
            }
            se_i++;
        }
    }
    if (matches_cnt == 0) {
        return source;
    }

    // use new string of the correct length
    size_t new_len = (so_len - matches_cnt * se_len + matches_cnt * re_len);
    char* result = get_zero_string(new_len);
    if (NULL == result) {
        return source;
    }

    size_t source_iterator = 0;
    size_t result_iterator = 0;
    for (size_t i = 0; i < matches_cnt; i++) {
        size_t match = matches[i];

        if (source_iterator < match) {
            size_t non_match_len = match - source_iterator;
            strncpy(result + result_iterator, source + source_iterator, non_match_len);
            result_iterator += non_match_len;
        }
        source_iterator = match + se_len;

        strncpy(result + result_iterator, replace, re_len);
        result_iterator += re_len;

    }
    // copy the remaining end of source
    if (source_iterator < so_len) {
        size_t remaining_len = so_len - source_iterator;
        strncpy(result + result_iterator, source + source_iterator, remaining_len);
        result_iterator += remaining_len;
    }
    // we free the old string mem
    free(source);
    return result;
}
