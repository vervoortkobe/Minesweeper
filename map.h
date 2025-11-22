#ifndef MINESWEEPER_MAP_H
#define MINESWEEPER_MAP_H

extern int map_w;
extern int map_h;
extern int map_mines;
int init_map(int w, int h, int mines);
void create_map();
void free_map();
/* Coordinate helpers belong to map since map dimensions are needed to
 * compute linear indices. Coord is a small POD used throughout the code.
 */
typedef struct { int x; int y; } Coord;

/* Compute linear index from Coord and width */
static inline int coord_index(Coord c, int map_w) { return c.y * map_w + c.x; }
#define COORD_IDX(c, w) ((c).y * (w) + (c).x)

/* If `exclude` is NULL no cell is excluded. Otherwise the Coord pointed
 * to by `exclude` will be skipped when placing the first mine.
 */
void add_mines(const Coord *exclude);
void fill_map();
extern char *map;

/* Existing access macro by column,row (c,r) left for compatibility. */
/* existing access macro by column,row (c,r) left for compatibility */
#define MAP(c, r) (map[(r) * map_w + (c)])

/* access map by Coord */
#define MAPC(coord) (map[COORD_IDX(coord, map_w)])

/* Convenience: access map using a Coord. Use e.g. `Coord p = {x,y}; MAPC(p)` */
#define MAPC(coord) (map[COORD_IDX(coord, map_w)])

void print_map();

#endif //MINESWEEPER_MAP_H