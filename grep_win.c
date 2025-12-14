#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <stdint.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#define PCRE2_STATIC
#include <pcre2.h>

int match_glob(const char *pattern, const char *string) {
    if (strchr(pattern, '*') == NULL && strchr(pattern, '?') == NULL) {
        return strcmp(pattern, string) == 0;
    }
    char *star = strchr(pattern, '*');
    if (star) {
        int prefix_len = star - pattern;
        if (strncmp(pattern, string, prefix_len) != 0) return 0;
        const char *suffix = star + 1;
        int suffix_len = strlen(suffix);
        int str_len = strlen(string);
        if (suffix_len == 0) return 1;
        if (str_len < prefix_len + suffix_len) return 0;
        return strcmp(string + str_len - suffix_len, suffix) == 0;
    }
    // for ?, simple char match
    int pat_len = strlen(pattern);
    int str_len = strlen(string);
    if (pat_len != str_len) return 0;
    for (int i = 0; i < pat_len; i++) {
        if (pattern[i] != '?' && pattern[i] != string[i]) return 0;
    }
    return 1;
}

int contains_ignore_case(const char *haystack, const char *needle) {
    size_t len = strlen(needle);
    for (const char *p = haystack; *p; p++) {
        if (_strnicmp(p, needle, len) == 0) return 1;
    }
    return 0;
}

#define MAX_PATTERNS 100
#define MAX_LINE 4096

typedef struct {
    char *text;
    int len;
    int is_match;
    int match_start;
    int match_end;
    size_t byte_offset;
} Line;

typedef struct {
    int ignore_case;
    int invert_match;
    int line_number;
    int list_files;
    int count;
    int no_filename;
    int with_filename;
    int recursive;
    char *patterns[MAX_PATTERNS];
    int num_patterns;
    char *pattern_file;
    int pattern_type; // 0 basic, 1 extended, 2 fixed, 3 perl
    int word_regexp;
    int line_regexp;
    int null_data;
    int no_messages;
    int max_count;
    int byte_offset;
    int line_buffered;
    char *label;
    int only_matching;
    int quiet;
    int binary_files_type; // 0 binary, 1 text, 2 without-match
    int directories_action; // 0 read, 1 recurse, 2 skip
    int devices_action; // 0 read, 1 skip
    int dereference_recursive;
    char *include_glob;
    char *exclude_glob;
    char *exclude_from;
    char *exclude_dir;
    int files_without_match;
    int initial_tab;
    int null_output;
    int before_context;
    int after_context;
    int context;
    char *group_separator;
    int no_group_separator;
    int color;
    int binary_option;
    int color_when; // 0 never, 1 always, 2 auto
    pcre2_code *code;
} Options;

void print_usage() {
    printf("Usage: grep [OPTION]... PATTERNS [FILE]...\n");
    printf("Search for PATTERNS in each FILE.\n");
    printf("Example: grep -i 'hello world' menu.h main.c\n");
    printf("PATTERNS can contain multiple patterns separated by newlines.\n");
    printf("\n");
    printf("Pattern selection and interpretation:\n");
    printf("  -E, --extended-regexp     PATTERNS are extended regular expressions\n");
    printf("  -F, --fixed-strings       PATTERNS are strings\n");
    printf("  -G, --basic-regexp        PATTERNS are basic regular expressions\n");
    printf("  -P, --perl-regexp         PATTERNS are Perl regular expressions\n");
    printf("  -e, --regexp=PATTERNS     use PATTERNS for matching\n");
    printf("  -f, --file=FILE           take PATTERNS from FILE\n");
    printf("  -i, --ignore-case         ignore case distinctions in patterns and data\n");
    printf("      --no-ignore-case      do not ignore case distinctions (default)\n");
    printf("  -w, --word-regexp         match only whole words\n");
    printf("  -x, --line-regexp         match only whole lines\n");
    printf("  -z, --null-data           a data line ends in 0 byte, not newline\n");
    printf("\n");
    printf("Miscellaneous:\n");
    printf("  -s, --no-messages         suppress error messages\n");
    printf("  -v, --invert-match        select non-matching lines\n");
    printf("  -V, --version             display version information and exit\n");
    printf("      --help                display this help text and exit\n");
    printf("\n");
    printf("Output control:\n");
    printf("  -m, --max-count=NUM       stop after NUM selected lines\n");
    printf("  -b, --byte-offset         print the byte offset with output lines\n");
    printf("  -n, --line-number         print line number with output lines\n");
    printf("      --line-buffered       flush output on every line\n");
    printf("  -H, --with-filename       print file name with output lines\n");
    printf("  -h, --no-filename         suppress the file name prefix on output\n");
    printf("      --label=LABEL         use LABEL as the standard input file name prefix\n");
    printf("  -o, --only-matching       show only nonempty parts of lines that match\n");
    printf("  -q, --quiet, --silent     suppress all normal output\n");
    printf("      --binary-files=TYPE   assume that binary files are TYPE;\n");
    printf("                            TYPE is 'binary', 'text', or 'without-match'\n");
    printf("  -a, --text                equivalent to --binary-files=text\n");
    printf("  -I                        equivalent to --binary-files=without-match\n");
    printf("  -d, --directories=ACTION  how to handle directories;\n");
    printf("                            ACTION is 'read', 'recurse', or 'skip'\n");
    printf("  -D, --devices=ACTION      how to handle devices, FIFOs and sockets;\n");
    printf("                            ACTION is 'read' or 'skip'\n");
    printf("  -r, --recursive           like --directories=recurse\n");
    printf("  -R, --dereference-recursive  likewise, but follow all symlinks\n");
    printf("      --include=GLOB        search only files that match GLOB (a file pattern)\n");
    printf("      --exclude=GLOB        skip files that match GLOB\n");
    printf("      --exclude-from=FILE   skip files that match any file pattern from FILE\n");
    printf("      --exclude-dir=GLOB    skip directories that match GLOB\n");
    printf("  -L, --files-without-match  print only names of FILEs with no selected lines\n");
    printf("  -l, --files-with-matches  print only names of FILEs with selected lines\n");
    printf("  -c, --count               print only a count of selected lines per FILE\n");
    printf("  -T, --initial-tab         make tabs line up (if needed)\n");
    printf("  -Z, --null                print 0 byte after FILE name\n");
    printf("\n");
    printf("Context control:\n");
    printf("  -B, --before-context=NUM  print NUM lines of leading context\n");
    printf("  -A, --after-context=NUM   print NUM lines of trailing context\n");
    printf("  -C, --context=NUM         print NUM lines of output context\n");
    printf("  -NUM                      same as --context=NUM\n");
    printf("      --group-separator=SEP  print SEP on line between matches with context\n");
    printf("      --no-group-separator  do not print separator for matches with context\n");
    printf("      --color[=WHEN],\n");
    printf("      --colour[=WHEN]       use markers to highlight the matching strings;\n");
    printf("                            WHEN is 'always', 'never', or 'auto'\n");
    printf("  -U, --binary              do not strip CR characters at EOL (MSDOS/Windows)\n");
    printf("\n");
    printf("When FILE is '-', read standard input.  With no FILE, read '.' if\n");
    printf("recursive, '-' otherwise.  With fewer than two FILEs, assume -h.\n");
    printf("Exit status is 0 if any line is selected, 1 otherwise;\n");
    printf("if any error occurs and -q is not given, the exit status is 2.\n");
    printf("\n");
    printf("Report bugs to: bug-grep@gnu.org\n");
    printf("GNU grep home page: <https://www.gnu.org/software/grep/>\n");
    printf("General help using GNU software: <https://www.gnu.org/gethelp/>\n");
}

int parse_options(int argc, char *argv[], Options *opts) {
    opts->ignore_case = 0;
    opts->invert_match = 0;
    opts->line_number = 0;
    opts->list_files = 0;
    opts->count = 0;
    opts->no_filename = 0;
    opts->with_filename = 0;
    opts->recursive = 0;
    opts->num_patterns = 0;
    opts->pattern_file = NULL;
    opts->pattern_type = 0; // basic
    opts->word_regexp = 0;
    opts->line_regexp = 0;
    opts->null_data = 0;
    opts->no_messages = 0;
    opts->max_count = -1;
    opts->byte_offset = 0;
    opts->line_buffered = 0;
    opts->label = NULL;
    opts->only_matching = 0;
    opts->quiet = 0;
    opts->binary_files_type = 0; // binary
    opts->directories_action = 0; // read
    opts->devices_action = 0; // read
    opts->dereference_recursive = 0;
    opts->include_glob = NULL;
    opts->exclude_glob = NULL;
    opts->exclude_from = NULL;
    opts->exclude_dir = NULL;
    opts->files_without_match = 0;
    opts->initial_tab = 0;
    opts->null_output = 0;
    opts->before_context = 0;
    opts->after_context = 0;
    opts->context = 0;
    opts->group_separator = "--";
    opts->no_group_separator = 0;
    opts->color = 0;
    opts->binary_option = 0;
    opts->color_when = 2; // auto
    opts->code = NULL;

    int i = 1;
    while (i < argc) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-i") == 0) {
                opts->ignore_case = 1;
            } else if (strcmp(argv[i], "-v") == 0) {
                opts->invert_match = 1;
            } else if (strcmp(argv[i], "-n") == 0) {
                opts->line_number = 1;
            } else if (strcmp(argv[i], "-l") == 0) {
                opts->list_files = 1;
            } else if (strcmp(argv[i], "-c") == 0) {
                opts->count = 1;
            } else if (strcmp(argv[i], "-h") == 0) {
                opts->no_filename = 1;
            } else if (strcmp(argv[i], "-H") == 0) {
                opts->with_filename = 1;
            } else if (strcmp(argv[i], "-r") == 0) {
                opts->recursive = 1;
            } else if (strcmp(argv[i], "-e") == 0) {
                i++;
                if (i >= argc) {
                    fprintf(stderr, "grep: option requires an argument -- 'e'\n");
                    return 1;
                }
                if (opts->num_patterns < MAX_PATTERNS) {
                    opts->patterns[opts->num_patterns++] = argv[i];
                }
            } else if (strcmp(argv[i], "-f") == 0) {
                i++;
                if (i >= argc) {
                    fprintf(stderr, "grep: option requires an argument -- 'f'\n");
                    return 1;
                }
                opts->pattern_file = argv[i];
            } else if (strcmp(argv[i], "-E") == 0) {
                opts->pattern_type = 1;
            } else if (strcmp(argv[i], "-F") == 0) {
                opts->pattern_type = 2;
            } else if (strcmp(argv[i], "-G") == 0) {
                opts->pattern_type = 0;
            } else if (strcmp(argv[i], "-P") == 0) {
                opts->pattern_type = 3;
            } else if (strcmp(argv[i], "-w") == 0) {
                opts->word_regexp = 1;
            } else if (strcmp(argv[i], "-x") == 0) {
                opts->line_regexp = 1;
            } else if (strcmp(argv[i], "-A") == 0) {
                i++;
                if (i >= argc) {
                    fprintf(stderr, "grep: option requires an argument -- 'A'\n");
                    return 1;
                }
                opts->after_context = atoi(argv[i]);
            } else if (strcmp(argv[i], "-B") == 0) {
                i++;
                if (i >= argc) {
                    fprintf(stderr, "grep: option requires an argument -- 'B'\n");
                    return 1;
                }
                opts->before_context = atoi(argv[i]);
            } else if (strcmp(argv[i], "-C") == 0) {
                i++;
                if (i >= argc) {
                    fprintf(stderr, "grep: option requires an argument -- 'C'\n");
                    return 1;
                }
                opts->context = atoi(argv[i]);
                opts->before_context = opts->context;
                opts->after_context = opts->context;
            } else if (argv[i][0] == '-' && isdigit(argv[i][1])) {
                opts->context = atoi(&argv[i][1]);
                opts->before_context = opts->context;
                opts->after_context = opts->context;
            } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--text") == 0) {
                opts->binary_files_type = 1; // text
            } else if (strcmp(argv[i], "-I") == 0) {
                opts->binary_files_type = 2; // without-match
            } else if (strcmp(argv[i], "-b") == 0) {
                opts->byte_offset = 1;
            } else if (strcmp(argv[i], "-o") == 0) {
                opts->only_matching = 1;
            } else if (strcmp(argv[i], "-m") == 0) {
                i++;
                if (i >= argc) {
                    fprintf(stderr, "grep: option requires an argument -- 'm'\n");
                    return 1;
                }
                opts->max_count = atoi(argv[i]);
            } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0 || strcmp(argv[i], "--silent") == 0) {
                opts->quiet = 1;
            } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--no-messages") == 0) {
                opts->no_messages = 1;
            } else if (strcmp(argv[i], "-T") == 0 || strcmp(argv[i], "--initial-tab") == 0) {
                opts->initial_tab = 1;
            } else if (strcmp(argv[i], "-Z") == 0 || strcmp(argv[i], "--null") == 0) {
                opts->null_output = 1;
            } else if (strcmp(argv[i], "-z") == 0 || strcmp(argv[i], "--null-data") == 0) {
                opts->null_data = 1;
            } else if (strcmp(argv[i], "-U") == 0 || strcmp(argv[i], "--binary") == 0) {
                opts->binary_option = 1;
            } else if (strcmp(argv[i], "--line-buffered") == 0) {
                opts->line_buffered = 1;
            } else if (strcmp(argv[i], "--color") == 0) {
                opts->color = 1;
                opts->color_when = 1; // always
            } else if (strncmp(argv[i], "--color=", 8) == 0) {
                opts->color = 1;
                if (strcmp(&argv[i][8], "always") == 0) opts->color_when = 1;
                else if (strcmp(&argv[i][8], "never") == 0) opts->color_when = 0;
                else if (strcmp(&argv[i][8], "auto") == 0) opts->color_when = 2;
            } else if (strcmp(argv[i], "--colour") == 0) {
                opts->color = 1;
                opts->color_when = 1;
            } else if (strncmp(argv[i], "--colour=", 9) == 0) {
                opts->color = 1;
                if (strcmp(&argv[i][9], "always") == 0) opts->color_when = 1;
                else if (strcmp(&argv[i][9], "never") == 0) opts->color_when = 0;
                else if (strcmp(&argv[i][9], "auto") == 0) opts->color_when = 2;
            } else if (strcmp(argv[i], "-D") == 0) {
                i++;
                if (i >= argc) {
                    fprintf(stderr, "grep: option requires an argument -- 'D'\n");
                    return 1;
                }
                if (strcmp(argv[i], "read") == 0) opts->devices_action = 0;
                else if (strcmp(argv[i], "skip") == 0) opts->devices_action = 1;
            } else if (strcmp(argv[i], "-d") == 0) {
                i++;
                if (i >= argc) {
                    fprintf(stderr, "grep: option requires an argument -- 'd'\n");
                    return 1;
                }
                if (strcmp(argv[i], "read") == 0) opts->directories_action = 0;
                else if (strcmp(argv[i], "recurse") == 0) opts->directories_action = 1;
                else if (strcmp(argv[i], "skip") == 0) opts->directories_action = 2;
            } else if (strcmp(argv[i], "-R") == 0 || strcmp(argv[i], "--dereference-recursive") == 0) {
                opts->recursive = 1;
                opts->dereference_recursive = 1;
            } else if (strcmp(argv[i], "--include") == 0) {
                i++;
                if (i >= argc) {
                    fprintf(stderr, "grep: option requires an argument -- '--include'\n");
                    return 1;
                }
                opts->include_glob = argv[i];
            } else if (strncmp(argv[i], "--include=", 10) == 0) {
                opts->include_glob = argv[i] + 10;
            } else if (strcmp(argv[i], "--exclude") == 0) {
                i++;
                if (i >= argc) {
                    fprintf(stderr, "grep: option requires an argument -- '--exclude'\n");
                    return 1;
                }
                opts->exclude_glob = argv[i];
            } else if (strncmp(argv[i], "--exclude=", 10) == 0) {
                opts->exclude_glob = argv[i] + 10;
            } else if (strcmp(argv[i], "--exclude-from") == 0) {
                i++;
                if (i >= argc) {
                    fprintf(stderr, "grep: option requires an argument -- '--exclude-from'\n");
                    return 1;
                }
                opts->exclude_from = argv[i];
            } else if (strncmp(argv[i], "--exclude-from=", 16) == 0) {
                opts->exclude_from = argv[i] + 16;
            } else if (strcmp(argv[i], "--exclude-dir") == 0) {
                i++;
                if (i >= argc) {
                    fprintf(stderr, "grep: option requires an argument -- '--exclude-dir'\n");
                    return 1;
                }
                opts->exclude_dir = argv[i];
            } else if (strncmp(argv[i], "--exclude-dir=", 14) == 0) {
                opts->exclude_dir = argv[i] + 14;
            } else if (strcmp(argv[i], "-L") == 0 || strcmp(argv[i], "--files-without-match") == 0) {
                opts->files_without_match = 1;
            } else if (strcmp(argv[i], "--group-separator") == 0) {
                i++;
                if (i >= argc) {
                    fprintf(stderr, "grep: option requires an argument -- '--group-separator'\n");
                    return 1;
                }
                opts->group_separator = argv[i];
            } else if (strcmp(argv[i], "--no-group-separator") == 0) {
                opts->no_group_separator = 1;
            } else if (strcmp(argv[i], "--label") == 0) {
                i++;
                if (i >= argc) {
                    fprintf(stderr, "grep: option requires an argument -- '--label'\n");
                    return 1;
                }
                opts->label = argv[i];
            } else if (strcmp(argv[i], "--binary-files") == 0) {
                i++;
                if (i >= argc) {
                    fprintf(stderr, "grep: option requires an argument -- '--binary-files'\n");
                    return 1;
                }
                if (strcmp(argv[i], "binary") == 0) opts->binary_files_type = 0;
                else if (strcmp(argv[i], "text") == 0) opts->binary_files_type = 1;
                else if (strcmp(argv[i], "without-match") == 0) opts->binary_files_type = 2;
            } else if (strcmp(argv[i], "--") == 0) {
                i++;
                break;
            } else {
                fprintf(stderr, "grep: invalid option -- '%s'\n", argv[i]);
                return 1;
            }
        } else {
            break;
        }
        i++;
    }

    if (opts->num_patterns == 0 && opts->pattern_file == NULL) {
        if (i >= argc) {
            print_usage();
            return 1;
        }
        opts->patterns[opts->num_patterns++] = argv[i];
        i++;
    }

    // Load patterns from file if specified
    if (opts->pattern_file) {
        FILE *fp = fopen(opts->pattern_file, "r");
        if (!fp) {
            perror("fopen");
            return 1;
        }
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), fp)) {
            size_t len = strlen(line);
            while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
                line[--len] = '\0';
            }
            if (opts->num_patterns < MAX_PATTERNS) {
                opts->patterns[opts->num_patterns++] = strdup(line);
            }
        }
        fclose(fp);
    }

    if (opts->pattern_type == 1 || opts->pattern_type == 3) {
        uint32_t options = PCRE2_UTF;
        if (opts->pattern_type == 1) options |= PCRE2_EXTENDED;
        if (opts->ignore_case) options |= PCRE2_CASELESS;
        char *pat = opts->patterns[0];
        char *mod_pat = NULL;
        if (opts->word_regexp) {
            mod_pat = malloc(strlen(pat) + 5);
            sprintf(mod_pat, "\\b%s\\b", pat);
            pat = mod_pat;
        }
        if (opts->line_regexp) {
            char *temp = malloc(strlen(pat) + 3);
            sprintf(temp, "^%s$", pat);
            if (mod_pat) free(mod_pat);
            pat = temp;
            mod_pat = temp;
        }
        PCRE2_SIZE erroroffset;
        int errorcode;
        opts->code = pcre2_compile((PCRE2_SPTR)pat, PCRE2_ZERO_TERMINATED, options, &errorcode, &erroroffset, NULL);
        if (!opts->code) {
            PCRE2_UCHAR buffer[256];
            pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
            fprintf(stderr, "pcre2_compile failed: %s\n", buffer);
            return 1;
        }
        if (mod_pat) free(mod_pat);
    }

    return 0;
}

int match_line(Options *opts, const char *line) {
    if (opts->pattern_type == 1 || opts->pattern_type == 3) {
        pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(opts->code, NULL);
        int rc = pcre2_match(opts->code, (PCRE2_SPTR)line, strlen(line), 0, 0, match_data, NULL);
        pcre2_match_data_free(match_data);
        return rc > 0;
    } else {
        for (int i = 0; i < opts->num_patterns; i++) {
            const char *pat = opts->patterns[i];
            if (opts->line_regexp) {
                int cmp = opts->ignore_case ? _stricmp(line, pat) : strcmp(line, pat);
                if (cmp == 0) {
                    return 1;
                }
            } else if (opts->word_regexp) {
                if (opts->ignore_case) {
                    char *line_lower = _strdup(line);
                    char *pat_lower = _strdup(pat);
                    for (char *p = line_lower; *p; p++) *p = tolower(*p);
                    for (char *p = pat_lower; *p; p++) *p = tolower(*p);
                    const char *pos = strstr(line_lower, pat_lower);
                    if (pos) {
                        size_t pat_len = strlen(pat);
                        size_t line_len = strlen(line);
                        size_t offset = pos - line_lower;
                        int start_ok = (offset == 0) || (!isalnum((unsigned char)line[offset-1]) && line[offset-1] != '_');
                        int end_ok = (offset + pat_len == line_len) || (!isalnum((unsigned char)line[offset + pat_len]) && line[offset + pat_len] != '_');
                        free(line_lower);
                        free(pat_lower);
                        if (start_ok && end_ok) {
                            return 1;
                        }
                    }
                    free(line_lower);
                    free(pat_lower);
                } else {
                    const char *pos = strstr(line, pat);
                    if (pos) {
                        size_t pat_len = strlen(pat);
                        size_t line_len = strlen(line);
                        int start_ok = (pos == line) || (!isalnum((unsigned char)pos[-1]) && pos[-1] != '_');
                        int end_ok = ((pos - line) + pat_len == line_len) || (!isalnum((unsigned char)pos[pat_len]) && pos[pat_len] != '_');
                        if (start_ok && end_ok) {
                            return 1;
                        }
                    }
                }
            } else {
                if (opts->ignore_case ? contains_ignore_case(line, pat) : (strstr(line, pat) != NULL)) {
                    return 1;
                }
            }
        }
        return 0;
    }
}

int process_file(const char *filename, Options *opts, int num_files, int print_filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        if (!opts->no_messages && errno != 0) perror(filename);
        return 0;
    }

    // Collect all lines
    Line *lines = NULL;
    int num_lines = 0;
    int capacity = 0;
    size_t current_offset = 0;

    if (opts->null_data) {
        // Read whole file and split by \0
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char *buffer = malloc(file_size + 1);
        if (!buffer) {
            perror("malloc");
            fclose(fp);
            return 0;
        }
        size_t read_size = fread(buffer, 1, file_size, fp);
        buffer[read_size] = '\0';

        char *start = buffer;
        char *end;
        while ((end = strchr(start, '\0')) != NULL) {
            size_t len = end - start;
            if (num_lines >= capacity) {
                capacity = capacity ? capacity * 2 : 1024;
                lines = realloc(lines, capacity * sizeof(Line));
            }
            lines[num_lines].text = malloc(len + 1);
            memcpy(lines[num_lines].text, start, len);
            lines[num_lines].text[len] = '\0';
            lines[num_lines].len = len;
            lines[num_lines].is_match = 0;
            lines[num_lines].match_start = -1;
            lines[num_lines].match_end = -1;
            lines[num_lines].byte_offset = current_offset;
            current_offset += len + 1; // +1 for \0
            num_lines++;
            start = end + 1;
            if (start >= buffer + read_size) break;
        }
        free(buffer);
    } else {
        char line_buf[MAX_LINE];
        while (fgets(line_buf, sizeof(line_buf), fp)) {
            if (num_lines >= capacity) {
                capacity = capacity ? capacity * 2 : 1024;
                lines = realloc(lines, capacity * sizeof(Line));
            }
            size_t len = strlen(line_buf);
            while (len > 0 && (line_buf[len-1] == '\n' || line_buf[len-1] == '\r')) {
                line_buf[--len] = '\0';
            }
            lines[num_lines].text = strdup(line_buf);
            lines[num_lines].len = len;
            lines[num_lines].is_match = 0;
            lines[num_lines].match_start = -1;
            lines[num_lines].match_end = -1;
            lines[num_lines].byte_offset = current_offset;
            current_offset += len;
            num_lines++;
        }
    }
    fclose(fp);

    // Check if binary
    int is_binary = 0;
    int found = 0;
    /*
    if (opts->binary_files_type == 0) {
        for (int i = 0; i < num_lines; i++) {
            for (size_t j = 0; j < lines[i].len; j++) {
                if (lines[i].text[j] == 0) {
                    is_binary = 1;
                    break;
                }
            }
            if (is_binary) break;
        }
    }
    */

    if (is_binary && opts->binary_files_type == 0) {
        if (!opts->quiet) fprintf(stderr, "Binary file %s matches\n", filename);
        for (int i = 0; i < num_lines; i++) free(lines[i].text);
        free(lines);
        return found;
    }

    // Find matches
    int match_count = 0;
    for (int i = 0; i < num_lines; i++) {
        int matches = match_line(opts, lines[i].text);
        if (opts->invert_match) {
            lines[i].is_match = !matches;
        } else {
            lines[i].is_match = matches;
        }
        if (lines[i].is_match) {
            match_count++;
        }
    }

    // Output
    if (opts->only_matching && (opts->before_context > 0 || opts->after_context > 0)) {
        fprintf(stderr, "grep: the -o option cannot be used with -A, -B, or -C\n");
        opts->before_context = opts->after_context = 0;
    }

    if (opts->list_files) {
        if ((opts->invert_match && match_count == 0) || (!opts->invert_match && match_count > 0)) {
            if (!opts->quiet) {
                printf("%s", filename);
                if (opts->null_output) printf("\0");
                printf("\n");
                found = 1;
            }
        }
    } else if (opts->files_without_match) {
        if (match_count == 0) {
            if (!opts->quiet) {
                printf("%s", filename);
                if (opts->null_output) printf("\0");
                printf("\n");
                found = 1;
            }
        }
    } else if (opts->count) {
        if (!opts->quiet) {
            if (print_filename) {
                printf("%s:", filename);
            }
            printf("%d\n", match_count);
            found = 1;
        }
    } else if (opts->only_matching) {
        if (!opts->quiet) {
            for (int i = 0; i < num_lines; i++) {
                if (lines[i].is_match) {
                    if (opts->pattern_type == 2) { // fixed
                        const char *pat = opts->patterns[0];
                        const char *pos = lines[i].text;
                        while ((pos = strstr(pos, pat)) != NULL) {
                            if (print_filename) {
                                printf("%s:", filename);
                            }
                            if (opts->line_number) {
                                printf("%d:", i + 1);
                            }
                            if (opts->byte_offset) {
                                printf("%zu:", lines[i].byte_offset + (pos - lines[i].text));
                            }
                            printf("%s\n", pat);
                            pos += strlen(pat);
                        }
                    } else if (opts->pattern_type == 1 || opts->pattern_type == 3) { // extended or perl
                        pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(opts->code, NULL);
                        size_t offset = 0;
                        while (1) {
                            int rc = pcre2_match(opts->code, (PCRE2_SPTR)lines[i].text, lines[i].len, offset, 0, match_data, NULL);
                            if (rc <= 0) break;
                            PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
                            size_t start = ovector[0];
                            size_t end = ovector[1];
                            if (print_filename) {
                                printf("%s:", filename);
                            }
                            if (opts->line_number) {
                                printf("%d:", i + 1);
                            }
                            if (opts->byte_offset) {
                                printf("%zu:", lines[i].byte_offset + start);
                            }
                            printf("%.*s\n", (int)(end - start), lines[i].text + start);
                            found = 1;
                            offset = end;
                            if (offset >= lines[i].len) break;
                        }
                        pcre2_match_data_free(match_data);
                    } else { // basic, treat as fixed
                        const char *pat = opts->patterns[0];
                        const char *pos = lines[i].text;
                        while ((pos = strstr(pos, pat)) != NULL) {
                            if (print_filename) {
                                printf("%s:", filename);
                            }
                            if (opts->line_number) {
                                printf("%d:", i + 1);
                            }
                            if (opts->byte_offset) {
                                printf("%zu:", lines[i].byte_offset + (pos - lines[i].text));
                            }
                            printf("%s\n", pat);
                            pos += strlen(pat);
                        }
                    }
                }
            }
        }
    } else {
        // Determine which lines to print
        int *print_lines = calloc(num_lines, sizeof(int));
        int has_context = opts->before_context > 0 || opts->after_context > 0;
        for (int i = 0; i < num_lines; i++) {
            if (lines[i].is_match) {
                print_lines[i] = 1;
                for (int j = i - opts->before_context; j <= i + opts->after_context; j++) {
                    if (j >= 0 && j < num_lines) {
                        print_lines[j] = 1;
                    }
                }
            }
        }

        // Output
        int in_group = 0;
        int printed_count = 0;
        for (int i = 0; i < num_lines; i++) {
            if (print_lines[i] && (opts->max_count == -1 || printed_count < opts->max_count)) {
                if (!opts->quiet) {
                    if (!in_group && has_context && !opts->no_group_separator) {
                        printf("%s\n", opts->group_separator);
                        in_group = 1;
                    }
                    if (print_filename) {
                        printf("%s:", filename);
                        if (opts->null_output) printf("\0");
                    }
                    if (opts->line_number) {
                        printf("%d:", i + 1);
                    }
                    if (opts->byte_offset) {
                        printf("%zu:", lines[i].byte_offset);
                    }
                    if (opts->initial_tab) {
                        // add spaces for alignment, but simple
                    }
                    // color
                    int is_tty = _isatty(_fileno(stdout));
                    int use_color = opts->color && (opts->color_when == 1 || (opts->color_when == 2 && is_tty));
                    if (use_color && lines[i].is_match) {
                        printf("\33[01;31m");
                    }
                    printf("%s", lines[i].text);
                    if (use_color && lines[i].is_match) {
                        printf("\33[0m");
                    }
                    printf("\n");
                    found = 1;
                }
                printed_count++;
            } else {
                in_group = 0;
            }
        }
        free(print_lines);
    }

    // Free lines
    for (int i = 0; i < num_lines; i++) {
        free(lines[i].text);
    }
    free(lines);
    return found;
}

int process_directory(const char *dirname, Options *opts, int num_files, int print_filename) {
    char path[1024];
    sprintf(path, "%s\\*", dirname);

    struct _finddata_t finddata;
    int found = 0;
    intptr_t handle = _findfirst(path, &finddata);
    if (handle == -1) {
        if (errno != 0) perror(dirname);
        return 0;
    }

    do {
        if (strcmp(finddata.name, ".") == 0 || strcmp(finddata.name, "..") == 0) continue;

        snprintf(path, sizeof(path), "%s\\%s", dirname, finddata.name);

        if (finddata.attrib & _A_SUBDIR) {
            int include_dir = 1;
            if (opts->exclude_dir && match_glob(opts->exclude_dir, finddata.name)) include_dir = 0;
            if (include_dir && opts->recursive) {
                found |= process_directory(path, opts, num_files, print_filename);
            }
        } else {
            int include = 1;
            if (opts->include_glob && !match_glob(opts->include_glob, finddata.name)) include = 0;
            if (opts->exclude_glob && match_glob(opts->exclude_glob, finddata.name)) include = 0;
            if (opts->exclude_from) {
                FILE *ef = fopen(opts->exclude_from, "r");
                if (ef) {
                    char buf[256];
                    while (fgets(buf, sizeof(buf), ef)) {
                        buf[strcspn(buf, "\r\n")] = 0;
                        if (match_glob(buf, finddata.name)) {
                            include = 0;
                            break;
                        }
                    }
                    fclose(ef);
                }
            }
            if (include) {
                found |= process_file(path, opts, num_files, print_filename);
            }
        }
    } while (_findnext(handle, &finddata) == 0);

    _findclose(handle);
    return found;
}

int process_input(Options *opts, int num_files) {
    if (opts->before_context || opts->after_context) {
        fprintf(stderr, "grep: context not supported with stdin\n");
        opts->before_context = opts->after_context = 0;
    }

    int line_num = 0;
    int match_count = 0;
    int found = 0;

    if (opts->null_data) {
        // Read all stdin, split by \0
        size_t capacity = 1024;
        size_t size = 0;
        char *buffer = malloc(capacity);
        if (!buffer) {
            perror("malloc");
            return 0;
        }
        int c;
        while ((c = getchar()) != EOF) {
            if (size >= capacity) {
                capacity *= 2;
                buffer = realloc(buffer, capacity);
                if (!buffer) {
                    perror("realloc");
                    return 0;
                }
            }
            buffer[size++] = c;
        }
        buffer[size] = '\0';

        char *start = buffer;
        char *end;
        while ((end = strchr(start, '\0')) != NULL) {
            size_t len = end - start;
            line_num++;
            int matches = match_line(opts, start);
            if (matches) {
                match_count++;
                if (!opts->list_files && !opts->count) {
                    if (opts->line_number) {
                        printf("%d:", line_num);
                    }
                    printf("%.*s\n", (int)len, start);
                }
            }
            start = end + 1;
            if (start >= buffer + size) break;
        }
        free(buffer);
    } else {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), stdin)) {
            line_num++;
            size_t len = strlen(line);
            while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
                line[--len] = '\0';
            }

            int matches = match_line(opts, line);
            if (matches) {
                match_count++;
                if (!opts->list_files && !opts->count) {
                    if (opts->line_number) {
                        printf("%d:", line_num);
                    }
                    printf("%s\n", line);
                }
            }
        }
    }

    if (opts->count) {
        printf("%d\n", match_count);
    }
}

int main(int argc, char *argv[]) {
    for (int j = 1; j < argc; j++) {
        if (strcmp(argv[j], "--help") == 0) {
            print_usage();
            return 0;
        } else if (strcmp(argv[j], "--version") == 0 || strcmp(argv[j], "-V") == 0) {
            printf("grep (GNU grep) 3.11 (Windows port)\n");
            printf("Copyright (C) 2023 Free Software Foundation, Inc.\n");
            printf("License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n");
            printf("This is free software: you are free to change and redistribute it.\n");
            printf("There is NO WARRANTY, to the extent permitted by law.\n");
            printf("\n");
            printf("Written by Mike Haertel and others (Windows port by Francesco Menghetti); see\n");
            printf("<https://git.savannah.gnu.org/cgit/grep.git/tree/AUTHORS>.\n");
            printf("\n");
            printf("grep -P uses PCRE2 10.47 2025-10-22\n");
            return 0;
        }
    }

    Options opts;
    if (parse_options(argc, argv, &opts)) {
        return 1;
    }

    int i = 1;
    while (i < argc) {
        if (argv[i][0] != '-') break;
        if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "-f") == 0) i += 2;
        else i++;
    }
    if (opts.num_patterns == 0 || (opts.pattern_file == NULL && opts.num_patterns == 1)) i++;

    int num_files = argc - i;
    int print_filename = (num_files > 1 && !opts.no_filename) || opts.with_filename;

    int any_matches = 0;

    if (num_files == 0) {
        any_matches = process_input(&opts, num_files);
    } else {
        for (int j = i; j < argc; j++) {
            const char *path = argv[j];
            DWORD attrib = GetFileAttributes(path);
            if (attrib == INVALID_FILE_ATTRIBUTES) {
                if (errno != 0) perror(path);
                continue;
            }
            if (attrib & FILE_ATTRIBUTE_DIRECTORY) {
                if (opts.recursive) {
                    any_matches |= process_directory(path, &opts, num_files, print_filename);
                } else {
                    fprintf(stderr, "grep: %s: Is a directory\n", path);
                }
            } else {
                any_matches |= process_file(path, &opts, num_files, print_filename);
            }
        }
    }

    return any_matches ? 0 : 1;
}