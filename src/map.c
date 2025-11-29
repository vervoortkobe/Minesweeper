#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "map.h"
#include "utils.h"

// Instantieer de standaardwaarden van het speelveld.
int map_width = 10;
int map_height = 10;
int map_mines = 10;
// Instantieer een speelveld als 2D array van Cell structs.
Cell **map = NULL;

/*
 * De waarden van w en h worden gecheckt of deze mogelijk zijn.
 * Zo ja, worden deze toegekend aan map_width en map_height.
 * We alloceren geheugen voor de standaard map van size map_width * map_height.
 */
int init_map(int w, int h, int mines)
{
    if (w <= 0 || h <= 0)
        return -1;
    map_width = w;
    map_height = h;
    map_mines = mines;
    if (map)
        free_map();

    // Allocate 2D array: array of pointers to rows
    map = (Cell **)malloc(h * sizeof(Cell *));
    if (!map)
        return -1;

    // Allocate each row
    for (int i = 0; i < h; i++)
    {
        map[i] = (Cell *)malloc(w * sizeof(Cell));
        if (!map[i])
        {
            // Free previously allocated rows on failure
            for (int j = 0; j < i; j++)
                free(map[j]);
            free(map);
            map = NULL;
            return -1;
        }
    }
    return 0;
}

// De aangemaakte map wordt opgevuld met Cell structs (zonder mijnen).
void create_map()
{
    if (map == NULL)
        init_map(map_width, map_height, map_mines);
    for (int y = 0; y < map_height; y++)
    {
        for (int x = 0; x < map_width; x++)
        {
            map[y][x].is_mine = false;
            map[y][x].neighbour_mines = 0;
        }
    }
}

// Om de map in de console uit te printen.
void print_map()
{
    for (int y = 0; y < map_height; y++)
    {
        for (int x = 0; x < map_width; x++)
        {
            if (map[y][x].is_mine)
                printf("M ");
            else
                printf("%d ", map[y][x].neighbour_mines);
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
    for (int y = 0; y < map_height; y++)
    {
        for (int x = 0; x < map_width; x++)
        {
            /*
             * Het aantal mijnen in de omgeving van een cell wordt opgeslagen in het `neighbour_mines` attribute van elke cell.
             * Cellen die zelf een mijn zijn, worden overgeslagen
             */
            if (map[y][x].is_mine)
                continue;
            int count = 0;
            for (int ay = -1; ay <= 1; ay++)
            {
                for (int ax = -1; ax <= 1; ax++)
                {
                    if (ax == 0 && ay == 0)
                        continue;
                    int by = y + ay, bx = x + ax;
                    if (by >= 0 && by < map_height && bx >= 0 && bx < map_width)
                    {
                        if (map[by][bx].is_mine)
                            count++;
                    }
                }
            }
            map[y][x].neighbour_mines = count;
        }
    }
}

/*
 * Na het klikken op een veld, wordt de map aangemaakt en random opgevuld met mijnen.
 * Daarna wordt de fill_map functie aangeroepen om de map verder op te vullen met nummers.
 */
void add_mines(int exclude_x, int exclude_y)
{
    srand((unsigned int)time(NULL));
    int placed = 0;
    while (placed < map_mines)
    {
        int x = rand() % map_width;
        int y = rand() % map_height;
        if (exclude_x >= 0 && x == exclude_x && y == exclude_y)
            continue;
        if (!map[y][x].is_mine)
        {
            map[y][x].is_mine = true;
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
        for (int i = 0; i < map_height; i++)
        {
            if (map[i])
                free(map[i]);
        }
        free(map);
        map = NULL;
    }
}