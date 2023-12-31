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
    uint32_t touched_days; //TODO define some sort of min time for a day to count touched, e.g. 15min
} sink_stats;

sink_id get_sink_id_by_name(sink_info** sinks, const char* name)
{
    //TODO improve efficiency
    for (size_t i = 0; i < VEC_LEN(&sinks); i++) {
        if (strcmp((*sinks)[i].name, name) == 0) {
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

void parse_timelog(
    const char* path,
    sink_info** sinks,
    sink_stats** stats,
    uint32_t* total_days
)
{
    //TODO days should be separated by sleeping, not time, i.e. working into the night until 02.00 the next day, should still count towards stats for the previous days e.g. 14 waking hours -- use either a predefined shift of the cutoff point (e.g. stat days switch at 04.00 not at 00.00), or some dynamic detection
    const uint32_t hour = 60;
    const uint32_t switch_time_minutes = 5.5 * hour;
    *total_days = 0;

    FILE* fh = fopen(path, "r");
    if (fh == NULL) {
        printf("error: could not open time log file\n");
        exit(1);
    }

    const uint32_t time_none = UINT32_MAX;
    uint32_t c_year = time_none;
    uint32_t c_month = time_none;
    uint32_t c_day = time_none;
    uint32_t c_24hr_minutes = time_none;

    sink_id* i_sinks = NULL;
    VEC_CREATE(&i_sinks, 4);

    char* line_buf = NULL;
    VEC_CREATE(&line_buf, 64);
    bool done = false;
    while (done == false) {
        VEC_SETLEN(&line_buf, 0);
        uint32_t time_len = 0;
        // getc until line is complete
        char in_c = '\0';
        while (true) {
            in_c = fgetc(fh);
            if (in_c == EOF) {
                if (VEC_LEN(&line_buf) > 0) {
                    printf("error: unexpected end of file\n");
                    exit(1);
                }
                done = true;
                break;
            }
            if (in_c == '\n') {
                VEC_PUSH(&line_buf, '\0');
                break;
            }
            if (in_c == ' ' && time_len == 0) {
                time_len = VEC_LEN(&line_buf);
            }
            VEC_PUSH(&line_buf, in_c);
        }
        if (time_len == 0) {
            time_len = VEC_LEN(&line_buf) - 1;
        }
        // line parsing
        if (done == false && VEC_LEN(&line_buf) > 1) {
            uint32_t i_year = time_none;
            uint32_t i_month = time_none;
            uint32_t i_day = time_none;
            uint32_t i_hour = time_none;
            uint32_t i_minute = time_none;
            switch (time_len) {
                case 16: {
                    int ec = sscanf(
                        line_buf,
                        "%u-%u-%u_%u.%u ",
                        &i_year,
                        &i_month,
                        &i_day,
                        &i_hour,
                        &i_minute
                    );
                    if (ec != 5) {
                        printf("error: parsing time 16\n");
                        exit(1);
                    }
                    //TODO check date integrity
                } break;
                case 10: {
                    int ec = sscanf(
                        line_buf,
                        "%u-%u-%u",
                        &i_year,
                        &i_month,
                        &i_day
                    );
                    if (ec != 3) {
                        printf("error: parsing time 10\n");
                        exit(1);
                    }
                    if (line_buf[time_len] != '\0') {
                        printf("error: no sinks allowed after date only time entry\n");
                        exit(1);
                    }
                    //TODO check date integrity
                } break;
                case 5: {
                    int ec = sscanf(line_buf, "%u.%u ", &i_hour, &i_minute);
                    if (ec != 2) {
                        printf("error: parsing time 5\n");
                        exit(1);
                    }
                } break;
                default: {
                    printf("error: unknown time format length\n");
                    exit(1);
                }
            }
            // fix time to proper minutes format
            uint32_t i_24hr_minutes = time_none;
            if (i_hour != time_none && i_minute != time_none) {
                i_24hr_minutes = i_hour * hour + i_minute;
            }
            if (i_24hr_minutes != time_none) {
                // add time to sinks
                uint32_t i_24_minutes_p1 = 0;
                uint32_t i_24_minutes_p2 = 0;
                if (c_24hr_minutes == time_none) {
                    c_24hr_minutes = i_24hr_minutes;
                } else if (i_24hr_minutes != time_none) {
                    // update sink stats
                    if (i_24hr_minutes > c_24hr_minutes) {
                        // total time moved
                        i_24_minutes_p1 = i_24hr_minutes - c_24hr_minutes;
                        // split to p2 if day switch passed
                        if (switch_time_minutes > c_24hr_minutes &&
                            switch_time_minutes < i_24hr_minutes) {
                            i_24_minutes_p2 = i_24hr_minutes - switch_time_minutes;
                            i_24_minutes_p1 -= i_24_minutes_p2;
                            *total_days += 1;
                        }
                    } else if (i_24hr_minutes < c_24hr_minutes) {
                        // total time moved
                        i_24_minutes_p1 =
                            ((24 * hour) - c_24hr_minutes) + i_24hr_minutes;
                        // split to p2 if day switch passed
                        if (switch_time_minutes > c_24hr_minutes) {
                            i_24_minutes_p2 = i_24_minutes_p1;
                            i_24_minutes_p1 = switch_time_minutes - c_24hr_minutes;
                            *total_days += 1;
                        } else if (switch_time_minutes < i_24hr_minutes) {
                            i_24_minutes_p2 = i_24hr_minutes - switch_time_minutes;
                            i_24_minutes_p1 -= i_24_minutes_p2;
                            *total_days += 1;
                        }
                    } else {
                        printf("error: 0 time interval\n");
                        exit(1);
                    }
                    if (i_24_minutes_p1 > 0) {
                        for (uint16_t sidx = 0; sidx < VEC_LEN(&i_sinks); sidx++) {
                            (*stats)[i_sinks[sidx]].total_minutes += i_24_minutes_p1;
                            if (i_24_minutes_p1 > 15) {
                                (*stats)[(*sinks)[i_sinks[sidx]].id].touched_days += 1;
                            }
                        }
                    }
                    if (i_24_minutes_p2 > 0) {
                        for (uint16_t sidx = 0; sidx < VEC_LEN(&i_sinks); sidx++) {
                            (*stats)[i_sinks[sidx]].total_minutes += i_24_minutes_p2;
                            if (i_24_minutes_p2 > 15) {
                                (*stats)[(*sinks)[i_sinks[sidx]].id].touched_days += 1;
                            }
                        }
                    }
                }
                c_24hr_minutes = i_24hr_minutes; // always update current time

                // get all activities here, if any, but do it after adding times to the last acitvities
                uint32_t read_len = time_len + 1;
                VEC_SETLEN(&i_sinks, 0);
                char sink_buf[SINK_NAME_MAX_LEN + 1];
                uint32_t sink_buf_len = 0;
                bool ndone = false;
                while (ndone == false) {
                    while (true) {
                        in_c = line_buf[read_len++];
                        if (in_c == EOF) {
                            printf("error: unexpected eof parsing sinks\n");
                            exit(1);
                        }
                        if (in_c == ' ') {
                            break;
                        }
                        if (in_c == '\0') {
                            ndone = true;
                            break;
                        }
                        sink_buf[sink_buf_len++] = in_c;
                    }
                    sink_buf[sink_buf_len] = '\0';
                    sink_buf_len = 0;
                    sink_id sid = get_sink_id_by_name(sinks, sink_buf);
                    if (sid == SINK_ID_NONE) {
                        printf("error: unknown sink\n");
                        exit(1);
                    }
                    VEC_PUSH(&i_sinks, sid);
                }
            }
        }
    }

    VEC_DESTROY(&i_sinks);

    fclose(fh);
}

int main(int argc, char** argv)
{
    if (argc == 1) {
        printf("usage: timetracking <sinkinfos.txt> <timefile.log> <mode> <content>\n");
        printf("\n");
        printf("modes:\n");
        printf("\t%-8s %s\n", "info", "shows sinks and descriptions");
        printf("\t%-8s %s\n", "push", "pushes the provided activities into the time log");
        printf("\t%-8s %s\n", "stats", "provides statistics about time spent on sinks");
        printf("\t%-8s %s\n", "now", "shows the currently ongoing activity");
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
        printf("sink info: (%lu)\n", VEC_LEN(&sinks));
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
    } else if (strcmp(tool_mode, "stats") == 0) {
        sink_stats* stats;
        VEC_CREATE(&stats, VEC_LEN(&sinks));
        for (size_t i = 0; i < VEC_LEN(&sinks); i++) {
            stats[i] = (sink_stats){
                .total_minutes = 0,
                .touched_days = 0,
            };
        }
        uint32_t total_days;
        parse_timelog(path_timefile, &sinks, &stats, &total_days);
        printf("sink stats: (%lu) %u days\n", VEC_LEN(&sinks), total_days);
        for (sink_id i = 0; i < VEC_LEN(&sinks); i++) {
            printf("\t%-*s :", SINK_NAME_MAX_LEN, sinks[i].name);
            printf(" %-4u", (uint32_t)((float)stats[i].total_minutes / (float)total_days)); // avg time per day
            printf(" %-4u", (uint32_t)((float)stats[i].total_minutes / (float)stats[i].touched_days)); // avg time per day touched
            printf(" %u", stats[i].total_minutes);
            printf(" %u", stats[i].touched_days);
            printf("\n");
        }
    } else if (strcmp(tool_mode, "now") == 0) {
        // display the last line in the timelog, to see which activity is currently running
        // this assumes that the last line (before the trailing new line) is always one containing activity content, and not just an empty date

        //TODO does not work when there is only one line, or no lines at all, as it doesnt find the previous line '\' in that case
        //TODO crashes on multiple empty lines at file end

        FILE* fh = fopen(path_timefile, "r");
        if (fh == NULL) {
            printf("error: could not open time log file\n");
            exit(1);
        }

        // is this a valid use of fseek?
        size_t line_len = 0;
        fseek(fh, -2, SEEK_END);
        while (true) {
            char rc = fgetc(fh);
            if (rc == '\n') {
                break;
            }
            line_len++;
            long fpos = ftell(fh);
            fseek(fh, fpos - 2, SEEK_SET);
        }

        char* line_buf = malloc(line_len + 1);
        fread(line_buf, 1, line_len, fh);
        line_buf[line_len] = '\0';

        const char* out_activities = line_buf;
        if (argc > 4 && strcmp(argv[4], "short") == 0) {
            while (*(out_activities++) != ' ') {
                // pass
            }
        }
        printf("%s\n", out_activities);

        free(line_buf);

        fclose(fh);
    } else {
        printf("error: unknown toolmode\n");
        exit(1);
    }

    return 0;
}
