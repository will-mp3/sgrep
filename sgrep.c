#include "list.h"
#include "mu.h"

#define _GNU_SOURCE

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// macro for USAGE output to be used with -h
#define USAGE                                                                                                    \
    "Usage: sgrep [-c] [-h] [-n] [-q] [-B NUM] STR FILE\n"                                                       \
    "\n"                                                                                                         \
    "Print lines in FILE that match STR.\n"                                                                      \
    "\n"                                                                                                         \
    "optional arguments\n"                                                                                       \
    "   -c, --count\n"                                                                                           \
    "       Print a count of matching lines for the input file. With the -v option, count non-matching lines.\n" \
    "\n"                                                                                                         \
    "   -h, --help\n"                                                                                            \
    "       Show usage statement and exit.\n"                                                                    \
    "\n"                                                                                                         \
    "   -n, --line-number\n"                                                                                     \
    "       Prefix each line of output with the 1-based line number of the file (e.g., 1:foo).\n"                \
    "\n"                                                                                                         \
    "   -q, --quiet\n"                                                                                           \
    "       Exit immediately if any match was found. If a match is not found, exit with a non-zero status.\n"    \
    "\n"                                                                                                         \
    "   -B NUM, --before-context NUM\n"                                                                          \
    "       Print NUM lines of leading context before matching lines.\n"                                         \
    "\n"

////////////////////////////////////////////////////////////////////////////////////////////

// linked list structs
struct line_node
{
    struct list_head list;
    char *line;
    int line_num;
};

struct queue
{
    struct list_head head;
    int size;
    int max_capacity;
};

static struct line_node *
node_new(char *line, int line_num)
{
    struct line_node *line_node;

    line_node = malloc(sizeof(*line_node));
    line_node->line = line;
    line_node->line_num = line_num;

    return line_node;
}

static void
line_node_free(struct line_node *line_node)
{
    free(line_node->line);
    free(line_node);
}

static void
list_init(struct queue *queue)
{
    INIT_LIST_HEAD(&queue->head);
    queue->size = 0;
}

static void
queue_print(const struct queue *queue)
{
    struct line_node *line_node;

    list_for_each_entry(line_node, &queue->head, list)
    {
        printf("%s", line_node->line);
    }
}

static void
num_queue_print(const struct queue *queue)
{
    struct line_node *line_node;

    list_for_each_entry(line_node, &queue->head, list)
    {
        printf("%d:%s", line_node->line_num, line_node->line);
    }
}

static void
queue_insert(struct queue *queue, struct line_node *line_node)
{
    list_add_tail(&line_node->list, &queue->head);
    queue->size += 1;
}

static struct line_node *
queue_remove(struct queue *queue)
{
    struct line_node *line_node;

    line_node = list_first_entry_or_null(&queue->head, struct line_node, list);
    if (line_node == NULL)
        mu_die("queue_remove: empty queue");

    list_del(&line_node->list);
    queue->size -= 1;

    return line_node;
}

static void
queue_deinit(struct queue *queue)
{
    struct line_node *line_node, *tmp;

    list_for_each_entry_safe(line_node, tmp, &queue->head, list)
    {
        list_del(&line_node->list);
        line_node_free(line_node);
    }

    queue->size = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////

// --help
static void usage(int status)
{
    puts(USAGE);
    exit(status);
}

// Read lines function
void read_lines(const char *str, const char *path, int count, int linenumber, int quiet, int beforecontext, int context_num)
{
    FILE *fh = fopen(path, "r");
    if (fh == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    char *line = NULL;
    size_t n = 0;
    int match_count = 0;
    int line_num = 1;

    struct queue context_queue;
    list_init(&context_queue);
    context_queue.max_capacity = context_num + 1;

    // quiet
    if (quiet)
    {
        while (getline(&line, &n, fh) != -1)
        {
            if (strstr(line, str) != NULL)
            {
                free(line);
                fclose(fh);
                exit(0);
            }
        }
        free(line);
        fclose(fh);
        exit(1);
    }

    // count
    if (count)
    {
        while (getline(&line, &n, fh) != -1)
        { // Check if the line contains the specified string
            if (strstr(line, str) != NULL)
            {
                match_count++;
            }
        }
        printf("%d\n", match_count);
        free(line);
        fclose(fh);
        if (match_count != 0)
        {
            exit(0);
        }
        else
        {
            exit(1);
        }
    }

    // before context
    if (beforecontext)
    {
        while (getline(&line, &n, fh) != -1)
        {
            struct line_node *new_node = node_new(strdup(line), line_num); // Create a new node with the current line

            queue_insert(&context_queue, new_node);

            if (context_queue.size > context_queue.max_capacity)
            {
                struct line_node *oldest_node = queue_remove(&context_queue);
                line_node_free(oldest_node);
            }

            if (strstr(line, str) != NULL)
            {
                if (linenumber)
                {
                    num_queue_print(&context_queue);
                }
                else
                {
                    queue_print(&context_queue);
                }
            }

            line_num++;
        }
        free(line);
        fclose(fh);
        queue_deinit(&context_queue);
        exit(1);
    }

    // standard output
    while (getline(&line, &n, fh) != -1)
    {
        if (strstr(line, str) != NULL)
        {
            if (linenumber)
            {
                printf("%d:", line_num);
            }
            printf("%s", line);
        }
        line_num++;
    }
    free(line);
    fclose(fh);
    exit(0);
}

// main
int main(int argc, char *argv[])
{
    int opt;
    int count = 0;
    int linenumber = 0;
    int quiet = 0;
    int beforecontext = 0;
    int context_num = 0;

    /*
     * An option that takes a required argument is followed by a ':'.
     * The leading ':' suppresses getopt_long's normal error handling.
     */

    const char *short_opts = ":hcnqB:";
    struct option long_opts[] = {
        {"help", no_argument, NULL, 'h'},
        {"count", no_argument, NULL, 'c'},
        {"line-number", no_argument, NULL, 'n'},
        {"quiet", no_argument, NULL, 'q'},
        {"before-context", required_argument, NULL, 'B'},
        {NULL, 0, NULL, 0}};

    while (1)
    {
        opt = getopt_long(argc, argv, short_opts, long_opts, NULL);
        if (opt == -1)
        {
            /* processed all command-line options */
            break;
        }

        // switch block, call external functions here
        switch (opt)
        {
        case 'h':
        {
            usage(0);
            break;
        }
        case 'c':
        {
            count = 1;
            break;
        }
        case 'n':
        {
            linenumber = 1;
            break;
        }
        case 'q':
        {
            quiet = 1;
            break;
        }
        case 'B':
        {
            beforecontext = 1;

            char *endptr;
            errno = 0;
            context_num = strtol(optarg, &endptr, 10);
            break;
        }
        case '?':
            mu_die("unknown option '%c' (decimal: %d)", optopt, optopt);
            break;
        case ':':
            mu_die("missing option argument for option %c", optopt);
            break;
        default:
            mu_die("unexpected getopt_long return value: %c\n", (char)opt);
        }
    }
    char *str = argv[optind];
    char *path = argv[optind + 1];
    read_lines(str, path, count, linenumber, quiet, beforecontext, context_num);
}