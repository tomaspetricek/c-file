#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

typedef struct
{
    FILE *handle;
} file_t;

typedef enum
{
    read_mode = 0,
    write_mode
} mode_t;

bool file_open(file_t *file, const char *path, const mode_t mode)
{
    static const char *modes[] = {"r", "w"};
    file->handle = fopen(path, modes[mode]);
    return file->handle != NULL;
}

bool file_read_line(file_t *file, char *buffer, size_t buffer_capacity)
{
    return fgets(buffer, buffer_capacity, file->handle) != NULL;
}

bool file_close(file_t *file) { return fclose(file->handle) != EOF; }

typedef struct
{
    char *data;
    int size;
} char_span_t;

void char_span_init(char_span_t *span, char *data, int size)
{
    span->data = data;
    span->size = size;
}

void char_span_copy(char_span_t *rhs, const char_span_t *lhs)
{
    rhs->data = lhs->data;
    rhs->size = lhs->size;
}

typedef struct
{
    int first;
    char sep;
    char_span_t line;
} csv_line_parser;

void csv_line_parser_init(csv_line_parser *parser, const char_span_t *line, char sep)
{
    parser->first = 0;
    parser->sep = sep;
    char_span_copy(&parser->line, line);
}

void csv_line_parser_get_value(csv_line_parser *parser, char_span_t *value)
{
    for (int last = parser->first; last < parser->line.size; ++last)
    {
        if (parser->line.data[last] == parser->sep || parser->line.data[last] == '\n')
        {
            char_span_init(value, parser->line.data + parser->first, last - parser->first);
            parser->first = last + 1;
            return;
        }
    }
    assert(1 != 1);
}

static const int name_capacity = 50;
typedef char name_t[name_capacity];

void name_copy_from_span(name_t *dest, const char_span_t *source)
{
    for (int i = 0; i < source->size; ++i)
    {
        (*dest)[i] = source->data[i];
    }
    (*dest)[source->size] = '\0';
}

void name_copy(name_t *dest, const name_t *src)
{
    int i = 0;
    while (i < name_capacity - 1 && (*src)[i] != '\0')
    {
        (*dest)[i] = (*src)[i];
        i++;
    }
    (*dest)[i] = '\0';
}

typedef struct
{
    name_t name;
    int age;
    int height;
} person_t;

void person_init(person_t *person, name_t *name, int age, int height)
{
    name_copy(&person->name, name);
    person->age = age;
    person->height = height;
}

void person_print(const person_t *person)
{
    printf("name: %s, age: %d, height: %d\n", person->name, person->age, person->height);
}

int to_integer(const char c)
{
    return c - '0';
}

int parse_integer(const char_span_t *token)
{
    static const int digit_count = 10;
    int num = 0;

    for (int i = 0; i < token->size; ++i)
    {
        num *= digit_count;
        num += to_integer(token->data[i]);
    }
    return num;
}

typedef struct
{
    int min_age;
    int max_age;
    int sum_age;
    int min_height;
    int max_height;
    int sum_height;
    int count;
} person_statistics_t;

int int_min(int first, int second)
{
    return (first < second) ? first : second;
}

int int_max(int first, int second)
{
    return (first > second) ? first : second;
}

void person_statistics_init(person_statistics_t *stats)
{
    stats->min_age = INT_MAX;
    stats->max_age = INT_MIN;
    stats->sum_age = 0;
    stats->min_height = INT_MAX;
    stats->max_height = INT_MIN;
    stats->sum_height = 0;
    stats->count = 0;
}

void person_statistics_update(person_statistics_t *stats, const person_t *person)
{
    stats->min_age = int_min(stats->min_age, person->age);
    stats->max_age = int_max(stats->max_age, person->age);
    stats->sum_age += person->age;
    stats->min_height = int_min(stats->min_height, person->height);
    stats->max_height = int_max(stats->max_height, person->height);
    stats->sum_height += person->height;
    stats->count++;
}

void person_statistics_print(const person_statistics_t *stats)
{
    printf("age: min: %d, max: %d, mean: %.2f\n", stats->min_age, stats->max_age, ((float)stats->sum_age) / stats->count);
    printf("height: min: %d, max: %d, mean: %.2f\n", stats->min_height, stats->max_height, ((float)stats->sum_height) / stats->count);
}

const int buffer_capacity = 256;

typedef struct
{
    file_t samples;
    char buffer[buffer_capacity];
    char sep;
    char_span_t line;
} csv_sample_reader_t;

bool csv_sample_reader_init(csv_sample_reader_t *reader, const char *samples_path, char sep)
{
    bool opened = file_open(&reader->samples, samples_path, read_mode);

    if (!opened)
    {
        fprintf(stderr, "ERROR: opening sample file: %s\n", strerror(errno));
        return opened;
    }
    reader->sep = sep;
    char_span_init(&reader->line, reader->buffer, buffer_capacity);
    return opened;
}

bool csv_sample_reader_read_header(csv_sample_reader_t *reader)
{
    bool read = file_read_line(&reader->samples, reader->buffer, buffer_capacity);

    if (!read)
    {
        if (feof(reader->samples.handle))
        {
            printf("ERROR: empty sample file provided\n");
        }
        else if (ferror(reader->samples.handle))
        {
            fprintf(stderr, "ERROR: reading header %s\n", strerror(errno));
        }
    }
    return read;
}

bool csv_sample_reader_read_sample(csv_sample_reader_t *reader, person_t *person)
{
    bool read = file_read_line(&reader->samples, reader->buffer, buffer_capacity);

    if (read)
    {
        printf("buffer: %s", reader->buffer);

        char_span_t value;
        csv_line_parser parser;

        int age, height;
        name_t name;

        csv_line_parser_init(&parser, &reader->line, reader->sep);

        csv_line_parser_get_value(&parser, &value);
        name_copy_from_span(&name, &value);

        csv_line_parser_get_value(&parser, &value);
        age = parse_integer(&value);

        csv_line_parser_get_value(&parser, &value);
        height = parse_integer(&value);

        person_init(person, &name, age, height);
    }
    else
    {
        if (feof(reader->samples.handle))
        {
            printf("INFO: no more samples to read\n");
        }
        else if (ferror(reader->samples.handle))
        {
            fprintf(stderr, "ERROR: reading samples: %s\n", strerror(errno));
        }
    }
    return read;
}

void csv_sample_reader_deinit(csv_sample_reader_t *reader)
{
}

int main()
{
    static const char *samples_path =
        "/Users/tomaspetricek/Documents/repos/c-file/data.txt";
    const char sep = ',';
    csv_sample_reader_t reader;

    if (csv_sample_reader_init(&reader, samples_path, sep))
    {
        if (csv_sample_reader_read_header(&reader))
        {
            printf("INFO: csv header read\n");

            person_statistics_t stats;
            person_statistics_init(&stats);
            person_t person;

            printf("INFO: started processing\n");

            while (csv_sample_reader_read_sample(&reader, &person))
            {
                person_print(&person);
                person_statistics_update(&stats, &person);
            }
            printf("INFO: finished processing\n");

            if (stats.count)
            {
                printf("INFO: statistics\n");
                person_statistics_print(&stats);
            }
        }
        else
        {
            printf("FAILED: cannot read header\n");
        }
    }
    else
    {
        printf("FAILED: cannot read sample\n");
    }
}
