#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

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

int main()
{
    static const char *samples_path =
        "/Users/tomaspetricek/Documents/repos/c-file/data.txt";
    file_t samples;

    if (file_open(&samples, samples_path, read_mode))
    {
        const int buffer_capacity = 256;
        char buffer[buffer_capacity];

        bool csv_header_read = false;

        if (file_read_line(&samples, buffer, buffer_capacity))
        {
            csv_header_read = true;
            printf("INFO: csv header read\n");
        }
        else
        {
            if (feof(samples.handle))
            {
                printf("ERROR: empty sample file provided\n");
            }
            else if (ferror(samples.handle))
            {
                fprintf(stderr, "ERROR: reading header %s\n", strerror(errno));
            }
        }

        if (csv_header_read)
        {
            char_span_t line;
            char_span_init(&line, buffer, buffer_capacity);
            int samples_processed = 0;
            bool processing = true;

            const char sep = ',';
            char_span_t value;
            csv_line_parser parser;

            int age, height;
            name_t name;
            person_t person;

            printf("INFO: started processing\n");

            while (processing)
            {
                if (file_read_line(&samples, buffer, buffer_capacity))
                {
                    printf("buffer: %s", buffer);
                    samples_processed++;

                    csv_line_parser_init(&parser, &line, sep);

                    csv_line_parser_get_value(&parser, &value);
                    name_copy_from_span(&name, &value);

                    csv_line_parser_get_value(&parser, &value);
                    age = parse_integer(&value);

                    csv_line_parser_get_value(&parser, &value);
                    height = parse_integer(&value);

                    person_init(&person, &name, age, height);
                    person_print(&person);

                    printf("\n");
                }
                else
                {
                    processing = false;

                    if (feof(samples.handle))
                    {
                        printf("INFO: no more samples to read\n");
                    }
                    else if (ferror(samples.handle))
                    {
                        fprintf(stderr, "ERROR: reading samples: %s\n", strerror(errno));
                    }
                }
            }
            printf("INFO: finished processing\n");
            printf("INFO: processed %d samples\n", samples_processed);
        }

        if (!file_close(&samples))
        {
            fprintf(stderr, "ERROR: closing sample file: %s\n", strerror(errno));
        }
    }
    else
    {
        fprintf(stderr, "ERROR: opening sample file: %s\n", strerror(errno));
    }
}
