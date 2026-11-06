/*
 * Path helper used by the image loader: "X:..." drive paths and paths that
 * start with a backslash are absolute.
 */

#include "rwsdk/rwcore.h"

RwBool _rwpathIsAbsolute(const RwChar* path) {
    if (path[1] == ':' && ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z'))) {
        return TRUE;
    }

    return path[0] == '\\';
}
