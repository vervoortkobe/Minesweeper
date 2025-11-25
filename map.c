#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "map.h"

// Instantieer de standaardwaarden voor het speelveld.
int map_w = 10;
int map_h = 10;
int map_mines = 10;
// Instantieer een standaard speelveld met een pointer naar Cell structs.
Cell *map = NULL;

/*
 * De waarden van w en h worden gecheckt of deze mogelijk zijn.
 * Zo ja, worden deze toegekend aan map_w en map_h.
 * We alloceren geheugen voor de standaard map van size map_w * map_h.
 */
int init_map(int w, int h, int mines)
{
    if (w <= 0 || h <= 0)
        return -1;
    map_w = w;
    map_h = h;
    map_mines = mines;
    if (map)
        free(map);
    map = (Cell *)malloc((int)map_w * (int)map_h * sizeof(Cell));
    if (!map)
        return -1; // memory allocation niet gelukt -> return -1
    return 0;
}

// De aanmgemaakte map wordt opgevuld met Cell structs (no mines, 0 adjacent).
void create_map()
{
    if (map == NULL)
        init_map(map_w, map_h, map_mines);
    for (int x = 0; x < map_w; x++)
    {
        for (int y = 0; y < map_h; y++)
        {
            Coord c = coord_make(x, y);
            MAP_AT(c).is_mine = false;
            MAP_AT(c).adjacent_mines = 0;
        }
    }
}

// Om de map in de console uit te printen.
void print_map()
{
    for (int y = 0; y < map_h; y++)
    {
        for (int x = 0; x < map_w; x++)
        {
            Coord c = coord_make(x, y);
            if (MAP_AT(c).is_mine)
                printf("M ");
            else
                printf("%d ", MAP_AT(c).adjacent_mines);
        }
        printf("\n");
    }
}

// Deze functie zal de map opvullen met nummers, rekening houdend met de reeds gelegde mijnen.
void fill_map()
{
    for (int x = 0; x < map_w; x++)
    {
        for (int y = 0; y < map_h; y++)
        {
            Coord c = coord_make(x, y);
            if (MAP_AT(c).is_mine)
                continue;
            int count = 0;
            for (int dy = -1; dy <= 1; dy++)
            {
                for (int dx = -1; dx <= 1; dx++)
                {
                    if (dx == 0 && dy == 0)
                        continue;
                    int ny = y + dy, nx = x + dx;
                    if (ny >= 0 && ny < map_h && nx >= 0 && nx < map_w)
                    {
                        Coord nc = coord_make(nx, ny);
                        if (MAP_AT(nc).is_mine)
                            count++;
                    }
                }
            }
            MAP_AT(c).adjacent_mines = count;
        }
    }
}

/*
 * Na het klikken op een veld, wordt de map aangemaakt en random opgevuld met mijnen.
 * Daarna wordt de fill_map functie aangeroepen om de map verder op te vullen met nummers.
 */
void add_mines(const Coord *exclude)
{
    int placed = 0;
    while (placed < map_mines)
    {
        int x = rand() % map_w;
        int y = rand() % map_h;
        if (exclude && exclude->x >= 0 && x == exclude->x && y == exclude->y)
            continue;
        Coord c = coord_make(x, y);
        if (!MAP_AT(c).is_mine)
        {
            MAP_AT(c).is_mine = true;
            placed++;
        }
    }
    fill_map();
}

// Om de map te dealloceren, nadat het spel afgelopen is.
void free_map()
{
    if (map)
    {
        free(map);
        map = NULL;
    }
}