/* CGA palette 1: indices 0..3 map to black, cyan, magenta and light gray.
 * Only used in Tandy 320x200x4 mode; 16-color mode uses mwstdpal4.
 */
#include "device.h"
const MWPALENTRY mwstdpal2[4] = {
    RGBDEF(0, 0, 0),
    RGBDEF(0, 170, 170),
    RGBDEF(170, 0, 170),
    RGBDEF(170, 170, 170)
};
