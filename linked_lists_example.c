#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "linked_lists.h"
#include "utils.h"

// ── Terminal helpers (cursor only, no colour) ─────────────────────────────────

#define DELAY_MS 700000   // 0.7 s between steps

#define CURSOR_UP(n)  printf("\033[%dA", (n))
#define CLEAR_BELOW   printf("\033[J")

// ── Visualization state ───────────────────────────────────────────────────────

#define MAX_NODES 16
#define LABEL_LEN 14   // inner label width (excludes border chars)

typedef enum { NODE_NORMAL, NODE_NEW, NODE_DELETED, NODE_FOUND } NodeState;

typedef struct {
    char      label[LABEL_LEN];
    NodeState state;
} VizNode;

static VizNode viz_nodes[MAX_NODES];
static int     viz_count = 0;

// Frame is always this many lines so CURSOR_UP math stays correct:
//   1  title
//   1  blank
//   1  top border
//   1  label
//   1  bottom border
//   1  index row
//   1  NULL arrow
//   1  blank
//   1  caption
#define DISPLAY_LINES 8

// State -> border corner and fill character
//   normal   +-- label --+
//   new      *== label ==*
//   deleted  X-- label --X
//   found    >-- label --<
static void box_chars(NodeState s, char *lc, char *rc, char *fill)
{
    switch (s)
    {
        case NODE_NEW:     *lc = '*'; *rc = '*'; *fill = '='; break;
        case NODE_DELETED: *lc = 'X'; *rc = 'X'; *fill = '-'; break;
        case NODE_FOUND:   *lc = '>'; *rc = '<'; *fill = '-'; break;
        default:           *lc = '+'; *rc = '+'; *fill = '-'; break;
    }
}

static void draw_list(const char *caption)
{
    int box_w = LABEL_LEN + 2;   // border + 1 space each side + label
    char lc, rc, fill;

    // title
    printf("  Linked List  (%d node%s)\n", viz_count, viz_count == 1 ? "" : "s");
    putchar('\n');

    if (viz_count == 0)
    {
        printf("  (empty)\n");
        printf("\n");
        printf("\n");
        printf("\n");
    }
    else
    {
        // top border
        printf("  ");
        for (int i = 0; i < viz_count; i++)
        {
            box_chars(viz_nodes[i].state, &lc, &rc, &fill);
            putchar(lc);
            for (int k = 0; k < LABEL_LEN; k++) putchar(fill);
            putchar(rc);
            if (i < viz_count - 1) printf("----");
        }
        putchar('\n');

        // label row
        printf("  ");
        for (int i = 0; i < viz_count; i++)
        {
            box_chars(viz_nodes[i].state, &lc, &rc, &fill);
            printf("%c %-*s %c", lc, LABEL_LEN - 2, viz_nodes[i].label, rc);
            if (i < viz_count - 1) printf("--->");
        }
        putchar('\n');

        // bottom border
        printf("  ");
        for (int i = 0; i < viz_count; i++)
        {
            box_chars(viz_nodes[i].state, &lc, &rc, &fill);
            putchar(lc);
            for (int k = 0; k < LABEL_LEN; k++) putchar(fill);
            putchar(rc);
            if (i < viz_count - 1) printf("----");
        }
        putchar('\n');

        // index labels, centred under each box, with --> NULL on the same line
        printf("  ");
        for (int i = 0; i < viz_count; i++)
        {
            int pad = box_w / 2 - 1;
            printf("%*s[%d]%*s", pad, "", i, box_w - pad - 3, "");
            if (i < viz_count - 1) printf("    ");
        }
        printf("----> NULL\n");
    }

    putchar('\n');
    printf("  >> %s\n", caption);

    usleep(DELAY_MS);
    CURSOR_UP(DISPLAY_LINES);
    CLEAR_BELOW;
}

// ── Viz helpers ───────────────────────────────────────────────────────────────

static void viz_insert(const char *label, int index, const char *caption)
{
    if (viz_count >= MAX_NODES) return;

    int pos = (index < 0 || index >= viz_count) ? viz_count : index;

    for (int i = viz_count; i > pos; i--)
        viz_nodes[i] = viz_nodes[i - 1];

    snprintf(viz_nodes[pos].label, LABEL_LEN, "%s", label);
    viz_nodes[pos].state = NODE_NEW;
    viz_count++;

    draw_list(caption);

    viz_nodes[pos].state = NODE_NORMAL;
}

static void viz_delete(int index, const char *caption)
{
    if (index < 0 || index >= viz_count) return;

    viz_nodes[index].state = NODE_DELETED;
    draw_list(caption);

    for (int i = index; i < viz_count - 1; i++)
        viz_nodes[i] = viz_nodes[i + 1];
    viz_count--;
}

static void viz_search(const char *label, int found, const char *caption)
{
    if (found)
    {
        for (int i = 0; i < viz_count; i++)
            if (strncmp(viz_nodes[i].label, label, LABEL_LEN - 1) == 0)
                viz_nodes[i].state = NODE_FOUND;
    }
    draw_list(caption);
    for (int i = 0; i < viz_count; i++)
        viz_nodes[i].state = NODE_NORMAL;
}

static void init_display(void)
{
    for (int i = 0; i < DISPLAY_LINES; i++) putchar('\n');
    CURSOR_UP(DISPLAY_LINES);
    CLEAR_BELOW;
}

// ── KeyValue helpers (unchanged) ──────────────────────────────────────────────

typedef struct KeyValue_t { void *data; void *key; } KeyValue;

void free_pair(void *pair)
{
    KeyValue *p = (KeyValue *)pair;
    free(p->key);
    default_free(p->data);
    free(pair);
}

void print_pair(const void *pair)
{
    KeyValue *p = (KeyValue *)pair;
    printf("%d %s\n", *(int *)p->data, (char *)p->key);
}

// ── main ──────────────────────────────────────────────────────────────────────

int main(void)
{
    LinkedList *list1 = LibLinkedList.create(NULL, 0, sizeof(char *));

    puts("=== Linked List Demo ===");
    puts("  *==new==*   >found<   XdelX   +normal+");
    putchar('\n');

    init_display();

    // ── Insertions ────────────────────────────────────────────────────────────

    LibLinkedList.insert_str(list1, "Hello World", -1);
    viz_insert("Hello World", -1, "insert_str(\"Hello World\", -1)  appended");

    LibLinkedList.insert_str(list1, "Hello World", -1);
    viz_insert("Hello World", -1, "insert_str(\"Hello World\", -1)  appended");

    LibLinkedList.insert_str(list1, "Jimbob", -1);
    viz_insert("Jimbob", -1, "insert_str(\"Jimbob\", -1)  appended");

    LibLinkedList.insert_str(list1, "Wagwan", -1);
    viz_insert("Wagwan", -1, "insert_str(\"Wagwan\", -1)  appended");

    // ── Search: found ─────────────────────────────────────────────────────────

    char *res = LibLinkedList.search_str(list1, "Jimbob");
    viz_search("Jimbob", res != NULL,
               res ? "search_str(\"Jimbob\")  FOUND" : "search_str(\"Jimbob\")  not found");

    // ── Search: not found ─────────────────────────────────────────────────────

    res = LibLinkedList.search_str(list1, "does not exist");
    viz_search("does not exist", res != NULL,
               res == NULL ? "search_str(\"does not exist\")  not found (correct)"
                           : "search_str(\"does not exist\")  unexpected hit");

    // ── Deletions ─────────────────────────────────────────────────────────────

    LibLinkedList.delete(list1, 0, free);
    viz_delete(0, "delete(0)  removed index 0");

    LibLinkedList.delete(list1, 1, free);
    viz_delete(1, "delete(1)  removed index 1");

    LibLinkedList.delete(list1, 2, free);   // invalid -- viz shows no change
    viz_search("", 0, "delete(2)  invalid index -- nothing removed");

    LibLinkedList.delete(list1, 0, free);
    viz_delete(0, "delete(0)  removed index 0");

    // final state
    draw_list("Done!");
    for (int i = 0; i < DISPLAY_LINES; i++) putchar('\n');

    LibLinkedList.free(list1, free);

    return EXIT_SUCCESS;
}
