#ifndef WIN_TILE_MANIFEST_H
#define WIN_TILE_MANIFEST_H

#include <stdlib.h>

#define WIN_TILE_FLAGS_SHOW_NAME 1
#define WIN_TILE_FLAGS_FOREGROUND_DARK 2

/**
 * @brief Manifest fields
 * 
 */
typedef struct win_tile_manifest
{
    struct 
    {
        unsigned char red;
        unsigned char green;
        unsigned char blue;
    } tile_color;           /** < The color of background */
    unsigned int flags;     /** < Specifies Show Name and Dark/Light theme */
    const char * logo150;   /** < Path to 150x150 logo, can be NULL if no logo
                             *    is to be used */ 
    const char * logo70;    /** < Path to 70x70 logo, if NULL the logo150 is
                             *    used */

} win_tile_manifest_t;


/**
 * @brief Create the visual elements manifest
 * 
 * @param[in] executable_file_name Path and name of the executable for which the
 *                                 manifest is to be created
 * @param[in] manifest Manifest fields
 * @return 0 on success
 */
int generate_win_tile_manifest(
    const char * executable_file_name,
    win_tile_manifest_t * manifest
);

#endif /* WIN_TILE_MANIFEST_H */

