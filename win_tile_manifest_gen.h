#ifndef WIN_TILE_MANIFEST_H
#define WIN_TILE_MANIFEST_H


#define WIN_TILE_FLAGS_SHOW_NAME 1
#define WIN_TILE_FLAGS_FOREGROUND_DARK 2

typedef struct win_tile_manifest
{
    struct 
    {
        unsigned char red;
        unsigned char green;
        unsigned char blue;
    } tile_color;
    unsigned int flags;
} win_tile_manifest_t;


void generate_win_tile_manifest(
    const char * executable_file_name,
    win_tile_manifest_t * manifest
);

#endif /* WIN_TILE_MANIFEST_H */

