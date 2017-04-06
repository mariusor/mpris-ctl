/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */

#include <string.h>

size_t zero_string(char* source, size_t length)
{
    if (NULL == source) return 0;
    size_t zeroes = 0;
    for (size_t i = 0; i < length; i++) {
        if (source[i]) { zeroes++; source[i] = (char)0; }
    }
    return zeroes;
}

char* get_zero_string(size_t length)
{
    char* result = (char*)malloc(sizeof(char*) * (length + 1));
    zero_string(result, length + 1);
    return result;
}

size_t str_replace(char* source, const char* search, const char* replace)
{
    if (NULL == source) { return 0; }
    size_t so_len = strlen(source);

    if (so_len == 0) { return 0; }

    if (NULL == search) { return so_len; }
    if (NULL == replace) { return so_len; }

    size_t se_len = strlen(search);
    if (se_len == 0) { return so_len; }

    size_t re_len = strlen(replace);

    if (se_len > so_len) { return so_len; }

    size_t max_matches = so_len / se_len;

    size_t matches[max_matches];
    size_t matches_cnt = 0;
    // get the number of matches for replace in source
    for (size_t so_i = 0; so_i < so_len - se_len + 1; so_i++) {
        size_t se_i = 0;
        size_t matched = 0;
        while (se_i < se_len) {
            if (search[se_i] == source[so_i + se_i]) {
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
        return so_len;
    }

    // use new string of the correct length
    size_t new_len = (so_len - matches_cnt * se_len + matches_cnt * re_len);
    char* result = get_zero_string(new_len);

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
    // copy back the temp string to source and free it
    source = realloc(source, new_len + 1);
    zero_string(source, new_len + 1);
    strncpy(source, result, new_len);

    if (result) { free(result); }

    return new_len;
}
