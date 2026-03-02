#include "bfs_and_dfs.h"
#include <unistd.h>  // for usleep

// ── Terminal helpers ──────────────────────────────────────────────────────────

#define DELAY_MS 1000000   // 0.4 s between steps

#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_DIM     "\033[2m"
#define ANSI_WHITE   "\033[97m"
#define ANSI_YELLOW  "\033[93m"
#define ANSI_GREEN   "\033[92m"
#define ANSI_CYAN    "\033[96m"
#define ANSI_BLUE    "\033[94m"
#define ANSI_GREY    "\033[90m"

// Move cursor up `n` lines and clear from cursor to end of screen
#define CURSOR_UP(n)    printf("\033[%dA", (n))
#define CLEAR_BELOW     printf("\033[J")

static const int EDGE_LIST[][2] = {
    {0,1},{0,3},
    {1,2},{1,3},{1,5},{1,6},
    {2,3},{2,4},{2,5},
    {3,4},
    {4,6}
};
static const int EDGE_COUNT = 11;

// Fixed (x, y) positions for each node in the ASCII canvas
//   canvas is 40 wide x 9 tall (rows 0..8, cols 0..39)
static const int NODE_COL[N] = { 4, 14, 24, 14,  34,  24,  34 };
static const int NODE_ROW[N] = { 2,  0,  0,  4,   0,   6,   6 };
static const int CANVAS_ROWS = 9;
static const int CANVAS_COLS = 42;

// Returns 1 if there's an edge between a and b in the edge list
static int has_edge(int a, int b)
{
    for (int i = 0; i < EDGE_COUNT; i++)
    {
        if ((EDGE_LIST[i][0] == a && EDGE_LIST[i][1] == b) ||
            (EDGE_LIST[i][0] == b && EDGE_LIST[i][1] == a))
            return 1;
    }
    return 0;
}

// Draw the graph to stdout.
//   visited[]    = nodes already processed (green)
//   inFrontier[] = nodes queued/stacked (yellow)
//   current      = node being expanded right now (cyan), -1 if none
static void draw_graph(int visited[N], int inFrontier[N], int current)
{
    // Build a char canvas
    char canvas[CANVAS_ROWS][CANVAS_COLS];
    memset(canvas, ' ', sizeof(canvas));
    for (int r = 0; r < CANVAS_ROWS; r++)
        canvas[r][CANVAS_COLS - 1] = '\0';

    // Draw edges as simple dashes / pipes between node positions
    // We just draw horizontal/vertical connectors for connected pairs
    for (int a = 0; a < N; a++)
    {
        for (int b = a + 1; b < N; b++)
        {
            if (!has_edge(a, b)) continue;

            int r1 = NODE_ROW[a], c1 = NODE_COL[a];
            int r2 = NODE_ROW[b], c2 = NODE_COL[b];

            // Step from (r1,c1) toward (r2,c2) one cell at a time
            int r = r1, c = c1;
            while (r != r2 || c != c2)
            {
                int dr = (r2 > r) - (r2 < r);
                int dc = (c2 > c) - (c2 < c);
                r += dr;
                c += dc;
                if (r == r2 && c == c2) break; // don't overwrite node
                if (dr == 0)
                    canvas[r][c] = '-';
                else if (dc == 0)
                    canvas[r][c] = '|';
                else
                    canvas[r][c] = (dr == dc) ? '\\' : '/';
            }
        }
    }

    // Print canvas row by row, inserting coloured node labels at their positions
    for (int r = 0; r < CANVAS_ROWS; r++)
    {
        int col_cursor = 0;

        // Check if any node lives on this row
        int node_cols[N];
        int node_on_row[N];
        for (int n = 0; n < N; n++)
        {
            node_on_row[n] = (NODE_ROW[n] == r);
            node_cols[n]   = NODE_COL[n];
        }

        while (col_cursor < CANVAS_COLS - 1)
        {
            // Check if a node starts here
            int found = -1;
            for (int n = 0; n < N; n++)
            {
                if (node_on_row[n] && node_cols[n] == col_cursor)
                {
                    found = n;
                    break;
                }
            }

            if (found >= 0)
            {
                const char *color;
                if (found == current)
                    color = ANSI_BOLD ANSI_CYAN;
                else if (visited[found])
                    color = ANSI_GREEN;
                else if (inFrontier[found])
                    color = ANSI_YELLOW;
                else
                    color = ANSI_DIM ANSI_WHITE;

                printf("%s(%d)%s", color, found, ANSI_RESET);
                col_cursor += 3; // "(d)" = 3 chars
            }
            else
            {
                char ch = canvas[r][col_cursor];
                if (ch == '-' || ch == '|' || ch == '\\' || ch == '/')
                    printf(ANSI_GREY "%c" ANSI_RESET, ch);
                else
                    putchar(' ');
                col_cursor++;
            }
        }
        putchar('\n');
    }
}

// Print the legend and step info, then sleep
static void print_step(int visited[N], int inFrontier[N], int current,
                        int step, const char *algo_name)
{
    printf(ANSI_BOLD "  %s  |  step %d  |  visiting: " ANSI_CYAN "%d" ANSI_RESET "\n",
           algo_name, step, current);
    printf(ANSI_GREEN "  (n)" ANSI_RESET " visited   "
           ANSI_YELLOW "(n)" ANSI_RESET " frontier   "
           ANSI_CYAN   "(n)" ANSI_RESET " current   "
           ANSI_DIM ANSI_WHITE "(n)" ANSI_RESET " unvisited\n" ANSI_RESET);
    printf("  visited order: ");
    for (int i = 0; i < N; i++)
        if (visited[i]) printf(ANSI_GREEN "%d " ANSI_RESET, i);
    printf("\n");

    const int INFO_LINES = 3;
    const int GRAPH_LINES = CANVAS_ROWS;
    usleep(DELAY_MS);

    // Move cursor back up to overwrite on the next step
    CURSOR_UP(INFO_LINES + GRAPH_LINES);
    CLEAR_BELOW;
}

// Reserve screen space before starting
static void init_display(void)
{
    for (int i = 0; i < CANVAS_ROWS + 3; i++) putchar('\n');
    CURSOR_UP(CANVAS_ROWS + 3);
    CLEAR_BELOW;
}

// ── main ──────────────────────────────────────────────────────────────────────

int main(void)
{
    int graph[N][N] = {
    //   0  1  2  3  4  5  6
        {0, 1, 0, 1, 0, 0, 0}, // 0
        {1, 0, 1, 1, 0, 1, 1}, // 1
        {0, 1, 0, 1, 1, 1, 0}, // 2
        {1, 1, 1, 0, 1, 0, 0}, // 3
        {0, 0, 1, 1, 0, 0, 1}, // 4
        {0, 1, 1, 0, 0, 0, 0}, // 5
        {0, 1, 0, 0, 1, 0, 0}  // 6
    };

    puts(ANSI_BOLD "=== DFS ===" ANSI_RESET);
    dfs(graph, 0);
    puts("");

    puts(ANSI_BOLD "=== BFS ===" ANSI_RESET);
    bfs(graph, 0);
    puts("");

    return EXIT_SUCCESS;
}

// ── DFS ───────────────────────────────────────────────────────────────────────

int dfs(int graph[N][N], int start)
{
    if (start >= N || start < 0)
        return EXIT_FAILURE;

    int visited[N]    = {0};
    int inFrontier[N] = {0};

    inFrontier[start] = 1;

    Stack *frontier = LibStack.create(NULL, 0, sizeof(int));
    LibStack.push_int(frontier, start);
    int currentNode = start;
    int step = 0;

    init_display();

    while (!LibStack.is_empty(frontier))
    {
        LibStack.pop(frontier, &currentNode);
        visited[currentNode]    = 1;
        inFrontier[currentNode] = 0;

        draw_graph(visited, inFrontier, currentNode);
        print_step(visited, inFrontier, currentNode, ++step, "DFS");

        int adjacent[N];
        int adjacentCount = 0;
        get_adjacent(graph, adjacent, &adjacentCount, currentNode);

        for (int i = 0; i < adjacentCount; i++)
        {
            if (visited[adjacent[i]] || inFrontier[adjacent[i]])
                continue;
            LibStack.push_int(frontier, adjacent[i]);
            inFrontier[adjacent[i]] = 1;
        }
    }

    // Final frame — no current node
    draw_graph(visited, inFrontier, -1);
    printf(ANSI_BOLD "  DFS complete in %d steps\n" ANSI_RESET, step);
    printf("\n\n");

    LibStack.free(frontier, NULL);
    return EXIT_SUCCESS;
}

// ── BFS ───────────────────────────────────────────────────────────────────────

int bfs(int graph[N][N], int start)
{
    if (start >= N || start < 0)
        return EXIT_FAILURE;

    int visited[N]    = {0};
    int inFrontier[N] = {0};

    inFrontier[start] = 1;

    Queue *frontier = LibQueue.create(NULL, 0, sizeof(int));
    LibQueue.enqueue_int(frontier, start);
    int currentNode = start;
    int step = 0;

    init_display();

    while (!LibQueue.is_empty(frontier))
    {
        LibQueue.dequeue(frontier, &currentNode);
        visited[currentNode]    = 1;
        inFrontier[currentNode] = 0;

        draw_graph(visited, inFrontier, currentNode);
        print_step(visited, inFrontier, currentNode, ++step, "BFS");

        int adjacent[N];
        int adjacentCount = 0;
        get_adjacent(graph, adjacent, &adjacentCount, currentNode);

        for (int i = 0; i < adjacentCount; i++)
        {
            if (visited[adjacent[i]] || inFrontier[adjacent[i]])
                continue;
            LibQueue.enqueue_int(frontier, adjacent[i]);
            inFrontier[adjacent[i]] = 1;
        }
    }

    draw_graph(visited, inFrontier, -1);
    printf(ANSI_BOLD "  BFS complete in %d steps\n" ANSI_RESET, step);
    printf("\n\n");

    LibQueue.free(frontier, NULL);
    return EXIT_SUCCESS;
}

// ── get_adjacent ──────────────────────────────────────────────────────────────

static int get_adjacent(int graph[N][N], int adjacent[N], int *count, int node)
{
    if (node >= N || node < 0)
        return EXIT_FAILURE;

    int j = 0;
    for (int i = 0; i < N; i++)
    {
        if (graph[node][i] == 1)
        {
            adjacent[j++] = i;
            (*count)++;
        }
    }
    return EXIT_SUCCESS;
}