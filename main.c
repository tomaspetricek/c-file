#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef enum
{
    no_error,
    parsing_error,
    end_of_file_error,
    reading_error,
    empty_buffer_error,
    invalid_value_error,
} error_t;

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
    bool opened = file->handle != NULL;

    if (!opened)
    {
        printf("ERROR: opening file: %s\n", strerror(errno));
    }
    return opened;
}

bool file_read_line(file_t *file, char *buffer, size_t buffer_capacity,
                    error_t *error)
{
    if ((fgets(buffer, buffer_capacity, file->handle) == NULL))
    {
        if (feof(file->handle))
        {
            printf("INFO: reached end of file\n");
            (*error) = end_of_file_error;
        }
        else if (ferror(file->handle))
        {
            fprintf(stderr, "ERROR: reading line: %s\n", strerror(errno));
            (*error) = reading_error;
        }
        return false;
    }
    return true;
}

bool file_close(file_t *file)
{
    if (fclose(file->handle) == EOF)
    {
        fprintf(stderr, "ERROR: closing file: %s\n", strerror(errno));
        return false;
    }
    return true;
}

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

void csv_line_parser_init(csv_line_parser *parser, const char_span_t *line,
                          char sep)
{
    parser->first = 0;
    parser->sep = sep;
    char_span_copy(&parser->line, line);
}

bool csv_line_parser_get_value(csv_line_parser *parser, char_span_t *value)
{
    if ((parser->first == parser->line.size) ||
        (parser->line.data[parser->first] == parser->sep) ||
        (parser->line.data[parser->first] == '\n'))
    {
        char_span_init(value, parser->line.data + parser->first, 0);
        return false;
    }
    for (int last = parser->first; last < parser->line.size; ++last)
    {
        if (parser->line.data[last] == parser->sep ||
            parser->line.data[last] == '\n')
        {
            char_span_init(value, parser->line.data + parser->first,
                           last - parser->first);
            parser->first = last + 1;
            return true;
        }
    }
    return false;
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
    printf("name: %s, age: %d, height: %d\n", person->name, person->age,
           person->height);
}

int to_integer(const char c) { return c - '0'; }

bool is_digit(const char c) { return ('0' <= c) && (c <= '9'); }

bool parse_integer(const char_span_t *token, int *num, error_t *error)
{
    static const int digit_count = 10;

    if (token->size == 0)
    {
        (*error) = empty_buffer_error;
        return false;
    }
    *num = 0;
    for (int i = 0; i < token->size; ++i)
    {
        if (!is_digit(token->data[i]))
        {
            (*error) = invalid_value_error;
            return false;
        }
        (*num) *= digit_count;
        (*num) += to_integer(token->data[i]);
    }
    return true;
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

int int_min(int first, int second) { return (first < second) ? first : second; }

int int_max(int first, int second) { return (first > second) ? first : second; }

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

void person_statistics_update(person_statistics_t *stats,
                              const person_t *person)
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
    printf("age: min: %d, max: %d, mean: %.2f\n", stats->min_age, stats->max_age,
           ((float)stats->sum_age) / stats->count);
    printf("height: min: %d, max: %d, mean: %.2f\n", stats->min_height,
           stats->max_height, ((float)stats->sum_height) / stats->count);
}

const int buffer_capacity = 256;

typedef struct
{
    file_t file;
    char buffer[buffer_capacity];
    char sep;
    char_span_t line;
} csv_sample_reader_t;

bool csv_sample_reader_init(csv_sample_reader_t *reader,
                            const char *samples_path, char sep)
{
    if (!file_open(&reader->file, samples_path, read_mode))
    {
        return false;
    }
    reader->sep = sep;
    char_span_init(&reader->line, reader->buffer, buffer_capacity);
    return true;
}

bool csv_sample_reader_read_header(csv_sample_reader_t *reader,
                                   error_t *error)
{
    return file_read_line(&reader->file, reader->buffer, buffer_capacity, error);
}

bool csv_sample_reader_read_sample(csv_sample_reader_t *reader,
                                   person_t *person, error_t *error)
{
    if (!file_read_line(&reader->file, reader->buffer, buffer_capacity, error))
    {
        return false;
    }
    printf("INFO: line read: %s", reader->buffer);

    char_span_t value;
    csv_line_parser parser;

    int age, height;
    name_t name;

    csv_line_parser_init(&parser, &reader->line, reader->sep);

    // parse name
    if (!csv_line_parser_get_value(&parser, &value))
    {
        printf("ERROR: missing name\n");
        (*error) = parsing_error;
        return false;
    }
    name_copy_from_span(&name, &value);

    // parse age
    if (!csv_line_parser_get_value(&parser, &value))
    {
        printf("ERROR: missing age\n");
        (*error) = parsing_error;
        return false;
    }
    if (!parse_integer(&value, &age, error))
    {
        printf("ERROR: parsing age from: %.*s\n", value.size, value.data);
        (*error) = parsing_error;
        return false;
    }

    // parse height
    if (!csv_line_parser_get_value(&parser, &value))
    {
        printf("ERROR: missing height");
        (*error) = parsing_error;
        return false;
    }
    if (!parse_integer(&value, &height, error))
    {
        printf("ERROR: parsing height from: %.*s\n", value.size, value.data);
        (*error) = parsing_error;
        return false;
    }

    person_init(person, &name, age, height);
    return true;
}

bool csv_sample_reader_deinit(csv_sample_reader_t *reader)
{
    return file_close(&reader->file);
}

int main()
{
    static const char *samples_path = "../data.txt";
    const char sep = ',';
    csv_sample_reader_t reader;
    error_t error = no_error;

    if (csv_sample_reader_init(&reader, samples_path, sep))
    {
        if (csv_sample_reader_read_header(&reader, &error))
        {
            printf("INFO: csv header read\n");

            person_statistics_t stats;
            person_statistics_init(&stats);
            person_t person;
            bool processing = true;

            printf("INFO: started processing\n");

            while (processing)
            {
                if (csv_sample_reader_read_sample(&reader, &person, &error))
                {
                    printf("INFO: sample read: ");
                    person_print(&person);
                    person_statistics_update(&stats, &person);
                }
                else
                {
                    if (error == end_of_file_error)
                    {
                        processing = false;
                        printf("INFO: no more samples to read\n");
                    }
                    else if (error == parsing_error)
                    {
                        printf("ERROR: parsing sample\n");
                    }
                    else if (error == reading_error)
                    {
                        processing = false;
                        printf("FATAL: reading sample\n");
                    }
                    else
                    {
                        processing = false;
                        printf("FATAL: unknown error when reading samples\n");
                    }
                }
            }
            printf("INFO: finished processing\n");

            if (stats.count)
            {
                printf("INFO: statistics\n");
                person_statistics_print(&stats);
            }

            if (!csv_sample_reader_deinit(&reader))
            {
                printf("ERROR: failed to deinitialize reader");
            }
        }
        else
        {
            if (error == end_of_file_error)
            {
                printf("ERROR: empty sample file provided\n");
            }
            else if (error == reading_error)
            {
                fprintf(stderr, "ERROR: reading header%s\n", strerror(errno));
            }
            printf("FATAL: unable to read header\n");
        }
    }
    else
    {
        printf("FATAL: cannot read samples\n");
    }
}
