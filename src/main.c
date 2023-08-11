#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ROSALIA_VECTOR_IMPLEMENTATION
#include "vector.h"

#define SINK_NAME_MAX_LEN 4

typedef uint16_t sink_id;
static const sink_id SINK_ID_NONE = UINT16_MAX;

typedef struct sink_info_s {
    sink_id id; //TODO might not need this?
    char name[SINK_NAME_MAX_LEN + 1];
    const char* desc;
    const char* comment;
} sink_info;

typedef struct sink_stats_s {
    uint32_t total_minutes;
    uint32_t touched_days;
} sink_stats;

sink_id get_sink_id_by_name(sink_info** sinks, const char* name)
{
    //TODO improve efficiency
    for (size_t i = 0; i < VEC_LEN(&sinks); i++) {
        if (strcmp(sinks[i]->name, name) == 0) {
            return i;
        }
    }
    return SINK_ID_NONE;
}

void parse_sinks(const char* path, sink_info** sinks)
{
    FILE* fh = fopen(path, "r");
    if (fh == NULL) {
        printf("error: could not open sink info file\n");
        exit(1);
    }

    typedef enum PARSE_STATE_E {
        PARSE_STATE_NAME = 0,
        PARSE_STATE_DESC,
        PARSE_STATE_COMMENT,
        PARSE_STATE_END_SINK,
        PARSE_STATE_DONE,
    } PARSE_STATE;

    PARSE_STATE pstate = PARSE_STATE_NAME;

    sink_id parse_sink_id = 0;
    char parse_sink_name[SINK_NAME_MAX_LEN + 1] = "\0";
    char* parse_sink_desc = NULL;
    char* parse_sink_comment = NULL;

    char in;
    while (pstate != PARSE_STATE_DONE) {
        switch (pstate) {
            case PARSE_STATE_NAME: {
                size_t str_len = 0;
                bool done = false;
                while (done == false) {
                    in = fgetc(fh);
                    switch (in) {
                        case EOF: {
                            if (str_len != 0) {
                                printf("error: unexpected eof"
                                       "while parsing name\n");
                                exit(1);
                            }
                            pstate = PARSE_STATE_DONE;
                            done = true;
                        } break;
                        case '\n': {
                            pstate = PARSE_STATE_END_SINK;
                            done = true;
                        } break;
                        case ' ': {
                            pstate = PARSE_STATE_DESC;
                            done = true;
                        } break;
                        case '#': {
                            if (str_len == 0) {
                                while (true) {
                                    in = fgetc(fh);
                                    if (in == EOF) {
                                        printf("error: unexpected eof while"
                                               "skipping full line comment\n");
                                        exit(1);
                                    }
                                    if (in == '\n') {
                                        break;
                                    }
                                }
                            } else {
                                pstate = PARSE_STATE_COMMENT;
                            }
                            done = true;
                        } break;
                        default: {
                            if (str_len >= SINK_NAME_MAX_LEN) {
                                printf("error: sink name too long\n");
                                exit(1);
                            }
                            parse_sink_name[str_len] = in;
                        } break;
                    }
                    str_len++;
                }
                parse_sink_name[str_len - 1] = '\0';
            } break;
            case PARSE_STATE_DESC: {
                VEC_CREATE(&parse_sink_desc, 64);
                bool done = false;
                while (done == false) {
                    in = fgetc(fh);
                    switch (in) {
                        case EOF: {
                            printf("error: unexpected eof"
                                   "while parsing desc\n");
                            exit(1);
                        } break;
                        case '\n': {
                            pstate = PARSE_STATE_END_SINK;
                            done = true;
                        } break;
                        case '#': {
                            pstate = PARSE_STATE_COMMENT;
                            done = true;
                        } break;
                        default: {
                            VEC_PUSH(&parse_sink_desc, in);
                        } break;
                    }
                }
                VEC_PUSH(&parse_sink_desc, '\0');
            } break;
            case PARSE_STATE_COMMENT: {
                VEC_CREATE(&parse_sink_comment, 64);
                bool done = false;
                while (done == false) {
                    in = fgetc(fh);
                    switch (in) {
                        case EOF: {
                            printf("error: unexpected eof"
                                   "while parsing comment\n");
                            exit(1);
                        } break;
                        case '\n': {
                            pstate = PARSE_STATE_END_SINK;
                            done = true;
                        } break;
                        default: {
                            VEC_PUSH(&parse_sink_comment, in);
                        } break;
                    }
                }
                VEC_PUSH(&parse_sink_comment, '\0');
            } break;
            case PARSE_STATE_END_SINK: {
                if (parse_sink_desc != NULL) {
                    while (true) {
                        if (parse_sink_desc[0] == ' ') {
                            VEC_REMOVE(&parse_sink_desc, 0);
                        } else {
                            break;
                        }
                    }
                    size_t str_end_len = strlen(parse_sink_desc);
                    while (str_end_len > 0) {
                        str_end_len--;
                        if (parse_sink_desc[str_end_len] == ' ') {
                            parse_sink_desc[str_end_len] = '\0';
                        } else {
                            break;
                        }
                    }
                    if (strlen(parse_sink_desc) == 0) {
                        VEC_DESTROY(&parse_sink_desc);
                    }
                }
                if (parse_sink_comment != NULL) {
                    while (true) {
                        if (parse_sink_comment[0] == ' ') {
                            VEC_REMOVE(&parse_sink_comment, 0);
                        } else {
                            break;
                        }
                    }
                    size_t str_end_len = strlen(parse_sink_comment);
                    while (str_end_len > 0) {
                        str_end_len--;
                        if (parse_sink_comment[str_end_len] == ' ') {
                            parse_sink_comment[str_end_len] = '\0';
                        } else {
                            break;
                        }
                    }
                    if (strlen(parse_sink_comment) == 0) {
                        VEC_DESTROY(&parse_sink_comment);
                    }
                }
                sink_info push_sink = (sink_info){
                    .id = parse_sink_id,
                    .name = "\0",
                    .desc = parse_sink_desc ? strdup(parse_sink_desc) : NULL,
                    .comment = parse_sink_comment
                                   ? strdup(parse_sink_comment)
                                   : NULL,
                };
                strcpy(push_sink.name, parse_sink_name);
                VEC_PUSH(sinks, push_sink);
                parse_sink_id++;
                parse_sink_name[0] = '\0';
                VEC_DESTROY(&parse_sink_desc);
                VEC_DESTROY(&parse_sink_comment);
                pstate = PARSE_STATE_NAME;
            } break;
            default: {
                assert(0);
            } break;
        }
    }

    fclose(fh);
}

int main(int argc, char** argv)
{
    if (argc == 1) {
        printf("usage: timetracking <sinkinfos.txt> <timefile.log> <mode> <content>\n");
        exit(0);
    }

    if (argc < 4) {
        printf("error: not enough arguments\n");
        exit(1);
    }

    const char* path_sinkinfos = argv[1];
    const char* path_timefile = argv[2];
    const char* tool_mode = argv[3];

    sink_info* sinks;
    VEC_CREATE(&sinks, 32);
    parse_sinks(path_sinkinfos, &sinks);

    if (strcmp(tool_mode, "info") == 0) {
        printf("sinks: #%lu\n", VEC_LEN(&sinks));
        for (sink_id i = 0; i < VEC_LEN(&sinks); i++) {
            printf("\t%-*s :", SINK_NAME_MAX_LEN, sinks[i].name);
            if (sinks[i].desc != NULL) {
                printf(" %s", sinks[i].desc);
            }
            if (sinks[i].comment != NULL) {
                printf(" # %s", sinks[i].comment);
            }
            printf("\n");
        }
    } else if (strcmp(tool_mode, "push") == 0) {
        FILE* fh = fopen(path_timefile, "a");
        if (fh == NULL) {
            printf("error: could not open time log file\n");
            exit(1);
        }

        // test that all specified sinks actually exist
        sink_id* sink_ids;
        VEC_CREATE(&sink_ids, 4);
        for (int argi = 4; argi < argc; argi++) {
            const char* in_sink_name = argv[argi];
            sink_id in_sink_id = get_sink_id_by_name(&sinks, in_sink_name);
            if (in_sink_id == SINK_ID_NONE) {
                printf("error: unknown sink id\n");
                exit(1);
            }
            VEC_PUSH(&sink_ids, in_sink_id);
        }

        // get time and writeout to file
        time_t rawtime;
        struct tm* timeinfo;
        char strbuf[32];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(strbuf, 32, "%Y-%m-%d_%H.%M", timeinfo);
        fprintf(fh, "%s", strbuf);
        for (size_t i = 0; i < VEC_LEN(&sink_ids); i++) {
            fprintf(fh, " %s", sinks[sink_ids[i]].name);
        }
        fprintf(fh, "\n");

        fclose(fh);
    } else if (strcmp(tool_mode, "review") == 0) {
        //TODO give review about how much time is spent on what activity per day on average
    } else {
        printf("error: unknown toolmode\n");
        exit(1);
    }

    return 0;
}
