#ifndef IDRAW_H
#define IDRAW_H

class iColor_tag
{
public:
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

class RwV3d
{
public:
    float x;
    float y;
    float z;
};

void iDrawEnd();
// GameCube signature inferred from xDrawLine's call (a stub in this build)
void iQueueLineList(iColor_tag color, RwV3d* verts, unsigned int* indices, unsigned int flags,
                    unsigned int numVerts, unsigned int numIndices, unsigned int numLines);
void iDrawBegin();

#endif
