/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <string.h>

#define MAX_OUTPUT_LENGTH 1024

void str_replace(char* source, const char* search, const char* replace)
{
    if (NULL == source) { return; }
    size_t so_len = strlen(source);

    if (so_len == 0) { return; }

    if (NULL == search) { return; }
    if (NULL == replace) { return; }

    size_t se_len = strlen(search);
    if (se_len == 0) { return; }

    size_t re_len = strlen(replace);

    if (se_len > so_len) { return; }

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
        return;
    }

    char result[MAX_OUTPUT_LENGTH] = {0};

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
    strncpy(source, result, MAX_OUTPUT_LENGTH);
}
