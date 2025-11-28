#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "map.h"
#include "utils.h"

// Instantieer de standaardwaarden van het speelveld.
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

// De aanmgemaakte map wordt opgevuld met Cell structs (zonder mijnen).
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
            MAP_AT(c).neighbour_mines = 0;
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
                printf("%d ", MAP_AT(c).neighbour_mines);
        }
        printf("\n");
    }
}

// Deze functie zal de map opvullen met nummers, rekening houdend met de reeds gelegde mijnen.
void fill_map()
{
    /*
     * We itereren over alle cellen op het speelveld.
     * Voor elke cell die geen mijn is, worden de acht aangrenzende velden gecontroleerd op mijnen.
     * Als er een mijn is gevonden, wordt de counter van de veld met 1 verhoogd.
     */
    for (int x = 0; x < map_w; x++)
    {
        for (int y = 0; y < map_h; y++)
        {
            /*
             * Het aantal mijnen in de omgeving van een cell wordt opgeslagen in het `neighbour_mines` attribute van elke cell.
             * Cellen die zelf een mijn zijn, worden overgeslagen
             */
            Coord c = coord_make(x, y);
            if (MAP_AT(c).is_mine)
                continue;
            int count = 0;
            for (int ay = -1; ay <= 1; ay++)
            {
                for (int ax = -1; ax <= 1; ax++)
                {
                    if (ax == 0 && ay == 0)
                        continue;
                    int by = y + ay, bx = x + ax;
                    if (by >= 0 && by < map_h && bx >= 0 && bx < map_w)
                    {
                        Coord nc = coord_make(bx, by);
                        if (MAP_AT(nc).is_mine)
                            count++;
                    }
                }
            }
            MAP_AT(c).neighbour_mines = count;
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