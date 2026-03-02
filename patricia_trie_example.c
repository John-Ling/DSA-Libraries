#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   /* usleep */
#include "patricia_trie.h"

/* ---------------------------------------------------------------
   ANSI escape helpers  (cursor / erase / style only, no colour)
   --------------------------------------------------------------- */
#define ANSI_CLEAR_SCREEN   "\033[2J"
#define ANSI_HOME           "\033[H"
#define ANSI_ERASE_LINE     "\033[2K"
#define ANSI_BOLD_ON        "\033[1m"
#define ANSI_BOLD_OFF       "\033[0m"
#define ANSI_BLINK_ON       "\033[5m"
#define ANSI_BLINK_OFF      "\033[25m"
#define ANSI_REVERSE_ON     "\033[7m"
#define ANSI_REVERSE_OFF    "\033[27m"

/* Delays (microseconds) */
#define DELAY_FAST    60000
#define DELAY_MED    180000
#define DELAY_SLOW   400000

static void move_to(int row, int col)
{
    printf("\033[%d;%dH", row, col);
    fflush(stdout);
}

static void pause_us(int us) { usleep(us); }

/* ---------------------------------------------------------------
   Trie printer
   --------------------------------------------------------------- */
static int g_print_row;

static void _print_node(PatriciaNode* node, int depth,
                         const char* prefix, int is_right,
                         PatriciaNode* visited[], int* vis_count)
{
    if (!node) return;

    for (int i = 0; i < *vis_count; i++) {
        if (visited[i] == node) {
            move_to(g_print_row++, 1);
            printf(ANSI_ERASE_LINE "%s%s", prefix, is_right ? "`-- " : "|-- ");
            if (node->mismatchIndex == -1)
                printf(ANSI_REVERSE_ON " LEAF \"%s\" <back-edge> " ANSI_REVERSE_OFF,
                       node->value ? node->value : "?");
            else
                printf("[INT bit=%d] <back-edge>", node->mismatchIndex);
            fflush(stdout);
            pause_us(DELAY_FAST);
            return;
        }
    }
    visited[(*vis_count)++] = node;

    move_to(g_print_row++, 1);
    printf(ANSI_ERASE_LINE "%s", prefix);
    if (depth > 0) printf("%s", is_right ? "`-- " : "|-- ");

    if (node->mismatchIndex == -1)
        printf(ANSI_REVERSE_ON " LEAF \"%s\" " ANSI_REVERSE_OFF,
               node->value ? node->value : "?");
    else
        printf(ANSI_BOLD_ON "[INT  bit=%d]" ANSI_BOLD_OFF, node->mismatchIndex);

    fflush(stdout);
    pause_us(DELAY_FAST);

    if (node->mismatchIndex != -1) {
        char np[512];
        snprintf(np, sizeof(np), "%s%s",
                 prefix, depth == 0 ? "" : (is_right ? "    " : "|   "));

        char lp[600], rp[600];
        snprintf(lp, sizeof(lp), "%s|   ", np);
        snprintf(rp, sizeof(rp), "%s    ", np);

        move_to(g_print_row++, 1);
        printf(ANSI_ERASE_LINE "%s|-- " ANSI_BOLD_ON "(bit=0)" ANSI_BOLD_OFF, np);
        fflush(stdout); pause_us(DELAY_FAST);
        _print_node(node->left,  depth+1, lp, 0, visited, vis_count);

        move_to(g_print_row++, 1);
        printf(ANSI_ERASE_LINE "%s`-- " ANSI_BOLD_ON "(bit=1)" ANSI_BOLD_OFF, np);
        fflush(stdout); pause_us(DELAY_FAST);
        _print_node(node->right, depth+1, rp, 1, visited, vis_count);
    }
}

static void draw_trie(PatriciaNode* root, int start_row)
{
    g_print_row = start_row;
    PatriciaNode* visited[256];
    int vis_count = 0;
    if (!root) {
        move_to(g_print_row++, 1);
        printf(ANSI_ERASE_LINE "  (empty)");
        fflush(stdout);
    } else {
        _print_node(root, 0, "  ", 0, visited, &vis_count);
    }
}

/* ---------------------------------------------------------------
   Banner
   --------------------------------------------------------------- */
static void animated_banner(const char* msg, int row)
{
    int len = (int)strlen(msg) + 4;
    move_to(row,   1); for (int i=0;i<len;i++) putchar('-');
    move_to(row+1, 1); printf("| " ANSI_BOLD_ON "%s" ANSI_BOLD_OFF " |", msg);
    move_to(row+2, 1); for (int i=0;i<len;i++) putchar('-');
    fflush(stdout);
    pause_us(DELAY_MED);
}

/* ---------------------------------------------------------------
   Bit-comparison table  (animated scan)
   --------------------------------------------------------------- */
static void draw_bit_comparison(const char* a, const char* b, int row)
{
    int limit = 32;

    move_to(row++, 1);
    printf(ANSI_ERASE_LINE "  Bit comparison: "
           ANSI_BOLD_ON "\"%s\"" ANSI_BOLD_OFF "  vs  "
           ANSI_BOLD_ON "\"%s\"" ANSI_BOLD_OFF, a, b);

    move_to(row++, 1);
    printf(ANSI_ERASE_LINE "  Bit# : ");
    for (int i = 0; i < limit; i++) printf("%d", i % 10);

    move_to(row, 1);
    printf(ANSI_ERASE_LINE "  %-6s: ", a);
    fflush(stdout);
    for (int i = 0; i < limit; i++) {
        unsigned int byte = i/8, off = 7-(i%8);
        printf("%d", (a[byte] >> off) & 1);
        fflush(stdout);
        pause_us(DELAY_FAST / 4);
    }
    row++;

    move_to(row, 1);
    printf(ANSI_ERASE_LINE "  %-6s: ", b);
    fflush(stdout);
    for (int i = 0; i < limit; i++) {
        unsigned int byte = i/8, off = 7-(i%8);
        printf("%d", (b[byte] >> off) & 1);
        fflush(stdout);
        pause_us(DELAY_FAST / 4);
    }
    row++;

    move_to(row, 1);
    printf(ANSI_ERASE_LINE "  diff  : ");
    fflush(stdout);
    int first_diff = -1;
    for (int i = 0; i < limit; i++) {
        int ba = (a[i/8] >> (7-(i%8))) & 1;
        int bb = (b[i/8] >> (7-(i%8))) & 1;
        if (ba != bb) {
            printf(ANSI_BLINK_ON "^" ANSI_BLINK_OFF);
            if (first_diff == -1) first_diff = i;
        } else {
            printf(" ");
        }
        fflush(stdout);
        pause_us(DELAY_FAST / 3);
    }
    row++;

    move_to(row, 1);
    if (first_diff >= 0)
        printf(ANSI_ERASE_LINE "  First differing bit: " ANSI_BOLD_ON "%d" ANSI_BOLD_OFF, first_diff);
    else
        printf(ANSI_ERASE_LINE "  Strings identical in compared range.");
    fflush(stdout);
    pause_us(DELAY_MED);
}

/* ---------------------------------------------------------------
   Animated search
   --------------------------------------------------------------- */
static void animated_search(PatriciaNode* root, const char* key, int row)
{
    move_to(row, 1);
    printf(ANSI_ERASE_LINE "  >> " ANSI_BLINK_ON "Searching..." ANSI_BLINK_OFF
           "  key = \"" ANSI_BOLD_ON "%s" ANSI_BOLD_OFF "\"", key);
    fflush(stdout);
    pause_us(DELAY_SLOW);

    PatriciaNode* found = pt_search_by_key(root, key);

    move_to(row, 1);
    printf(ANSI_ERASE_LINE);
    if (found && found->value)
        printf("  >> " ANSI_REVERSE_ON " FOUND \"%s\" " ANSI_REVERSE_OFF, found->value);
    else
        printf("  >> Not found.");
    fflush(stdout);
    pause_us(DELAY_SLOW);
}

/* ---------------------------------------------------------------
   Layout rows
   --------------------------------------------------------------- */
#define ROW_BANNER   1
#define ROW_STATUS   5
#define ROW_TREE     7
#define ROW_BITCMP  27
#define ROW_SEARCH  36

static void clear_region(int from, int to)
{
    for (int r = from; r <= to; r++) {
        move_to(r, 1);
        printf(ANSI_ERASE_LINE);
    }
    fflush(stdout);
}

/* ---------------------------------------------------------------
   Main
   --------------------------------------------------------------- */
int main(void)
{
    printf("\033[?25l");  /* hide cursor */
    printf(ANSI_CLEAR_SCREEN ANSI_HOME);
    fflush(stdout);

    PatriciaNode* root = NULL;

    /* ============================================================
       DEMO 1  --  single-char keys a..e
       ============================================================ */
    animated_banner("Demo 1: Single-character keys  'a' -> 'e'", ROW_BANNER);

    root = pt_create("a");
    move_to(ROW_STATUS, 1);
    printf(ANSI_ERASE_LINE "Initial tree after create(\"a\"):");
    fflush(stdout);
    clear_region(ROW_TREE, ROW_TREE+18);
    draw_trie(root, ROW_TREE);
    pause_us(DELAY_SLOW);

    const char* singles[] = { "b", "c", "d", "e" };
    for (int i = 0; i < 4; i++) {
        move_to(ROW_STATUS, 1);
        printf(ANSI_ERASE_LINE "Inserting \"" ANSI_BOLD_ON "%s" ANSI_BOLD_OFF "\"...", singles[i]);
        fflush(stdout);
        pause_us(DELAY_MED);

        root = pt_insert(root, singles[i], (int)(strlen(singles[i])*8+8));

        move_to(ROW_STATUS, 1);
        printf(ANSI_ERASE_LINE "Inserted \"" ANSI_BOLD_ON "%s" ANSI_BOLD_OFF "\"  -- tree:", singles[i]);
        fflush(stdout);

        clear_region(ROW_TREE, ROW_TREE+18);
        draw_trie(root, ROW_TREE);
        pause_us(DELAY_SLOW);
    }

    animated_banner("Demo 1: Searching", ROW_BANNER);
    const char* s1[] = { "a", "c", "e" };
    for (int i = 0; i < 3; i++) {
        clear_region(ROW_SEARCH, ROW_SEARCH+2);
        animated_search(root, s1[i], ROW_SEARCH);
        pause_us(DELAY_MED);
    }

    pt_free(root); root = NULL;
    pause_us(DELAY_SLOW);

    /* ============================================================
       DEMO 2  --  prefix keys: dog / doggo / doge ...
       ============================================================ */
    printf(ANSI_CLEAR_SCREEN ANSI_HOME);
    animated_banner("Demo 2: Prefix keys  dog / doggo / doge / doggone / wagwan", ROW_BANNER);

    root = pt_create("dog");
    move_to(ROW_STATUS, 1);
    printf(ANSI_ERASE_LINE "Initial tree after create(\"dog\"):");
    fflush(stdout);
    clear_region(ROW_TREE, ROW_TREE+18);
    draw_trie(root, ROW_TREE);
    pause_us(DELAY_SLOW);

    const char* words[] = { "doggo", "doge", "doggone", "wagwan" };
    const char* cmp_base = "dog";

    for (int i = 0; i < 4; i++) {
        clear_region(ROW_BITCMP, ROW_BITCMP+8);
        draw_bit_comparison(cmp_base, words[i], ROW_BITCMP);
        cmp_base = words[i];

        move_to(ROW_STATUS, 1);
        printf(ANSI_ERASE_LINE "Inserting \"" ANSI_BOLD_ON "%s" ANSI_BOLD_OFF "\"...", words[i]);
        fflush(stdout);
        pause_us(DELAY_MED);

        root = pt_insert(root, words[i], (int)(strlen(words[i])*8+8));

        move_to(ROW_STATUS, 1);
        printf(ANSI_ERASE_LINE "Inserted \"" ANSI_BOLD_ON "%s" ANSI_BOLD_OFF "\"  -- tree:", words[i]);
        fflush(stdout);

        clear_region(ROW_TREE, ROW_TREE+18);
        draw_trie(root, ROW_TREE);
        pause_us(DELAY_SLOW);
    }

    animated_banner("Demo 2: Searching prefix keys", ROW_BANNER);
    const char* s2[] = { "doge", "dog", "doggo", "doggone", "wagwan" };
    for (int i = 0; i < 5; i++) {
        clear_region(ROW_SEARCH, ROW_SEARCH+2);
        animated_search(root, s2[i], ROW_SEARCH);
        pause_us(DELAY_MED);
    }

    pt_free(root); root = NULL;
    pause_us(DELAY_SLOW);

    /* ============================================================
       DEMO 3  --  long shared prefixes: test / testing / tested ...
       ============================================================ */
    printf(ANSI_CLEAR_SCREEN ANSI_HOME);
    animated_banner("Demo 3: Long shared prefixes  test / testing / tested ...", ROW_BANNER);

    root = pt_create("test");
    move_to(ROW_STATUS, 1);
    printf(ANSI_ERASE_LINE "Initial tree after create(\"test\"):");
    fflush(stdout);
    clear_region(ROW_TREE, ROW_TREE+18);
    draw_trie(root, ROW_TREE);
    pause_us(DELAY_SLOW);

    const char* prefixed[] = { "testing", "tested", "tester", "tests" };
    for (int i = 0; i < 4; i++) {
        clear_region(ROW_BITCMP, ROW_BITCMP+8);
        draw_bit_comparison("test", prefixed[i], ROW_BITCMP);

        move_to(ROW_STATUS, 1);
        printf(ANSI_ERASE_LINE "Inserting \"" ANSI_BOLD_ON "%s" ANSI_BOLD_OFF "\"...", prefixed[i]);
        fflush(stdout);
        pause_us(DELAY_MED);

        root = pt_insert(root, prefixed[i], (int)(strlen(prefixed[i])*8+8));

        move_to(ROW_STATUS, 1);
        printf(ANSI_ERASE_LINE "Inserted \"" ANSI_BOLD_ON "%s" ANSI_BOLD_OFF "\"  -- tree:", prefixed[i]);
        fflush(stdout);

        clear_region(ROW_TREE, ROW_TREE+18);
        draw_trie(root, ROW_TREE);
        pause_us(DELAY_SLOW);
    }

    animated_banner("Demo 3: Searching", ROW_BANNER);
    const char* s3[] = { "test", "testing", "tested", "tester", "tests" };
    for (int i = 0; i < 5; i++) {
        clear_region(ROW_SEARCH, ROW_SEARCH+2);
        animated_search(root, s3[i], ROW_SEARCH);
        pause_us(DELAY_MED);
    }

    pt_free(root); root = NULL;

    /* ============================================================
       Done
       ============================================================ */
    printf(ANSI_CLEAR_SCREEN ANSI_HOME);
    move_to(2, 1);
    printf(ANSI_BOLD_ON "All demos complete." ANSI_BOLD_OFF "\n\n");
    printf("\033[?25h");  /* restore cursor */
    fflush(stdout);

    return EXIT_SUCCESS;
}