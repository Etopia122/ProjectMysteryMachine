// Small game functions that are still unsplit assembly in the decomp
// (fn_XXXXXXXX gaps). Written from the GameCube assembly.

class xMorphSeqFile
{
public:
    unsigned int Magic;
    unsigned int Flags;
    unsigned int TimeCount;
    unsigned int ModelCount;
};

// 0x80030F28: duration = last entry of the time list that follows the header.
float xMorphSeqDuration(xMorphSeqFile* seq)
{
    return ((float*)(seq + 1))[seq->TimeCount - 1];
}

// ---------------------------------------------------------------- iBoxForModel (0x80007738/0x800077C0)
class _xVec3
{
public:
    float x, y, z;
};
class _xBox
{
public:
    _xVec3 upper, lower;
};
class xModelInstance;
void iBoxInitBoundVec(_xBox* b, const _xVec3* v);
void iBoxBoundVec(_xBox* o, const _xBox* b, const _xVec3* v);

struct PcMorphTarget { void* parentGeom; float sphere[4]; _xVec3* verts; _xVec3* normals; };
struct PcGeometryView { unsigned char pad[0x14]; int numVertices; int numMorphTargets; unsigned char pad2[0x68 - 0x1C]; PcMorphTarget* morphTarget; };
struct PcAtomicView { unsigned char pad[0x18]; PcGeometryView* geometry; };
struct PcModelView { unsigned char pad[0x10]; PcAtomicView* Data; unsigned char pad2[0x20 - 0x14]; float* Mat; };

// Box around the model's vertices in model space.
extern "C" void fn_80007738(void* bound, xModelInstance* model);
extern "C" void fn_800077C0(void* bound, xModelInstance* model);

void fn_80007738(void* bound, xModelInstance* model)
{
    _xBox* box = (_xBox*)bound;
    PcGeometryView* geom = ((PcModelView*)model)->Data->geometry;
    if (geom->numMorphTargets >= 1)
    {
        _xVec3* verts = geom->morphTarget->verts;
        iBoxInitBoundVec(box, &verts[0]);
        for (unsigned short i = 1; i < geom->numVertices; i++)
        {
            iBoxBoundVec(box, box, &verts[i]);
        }
    }
}

// Same, offset by the model's position.
void fn_800077C0(void* bound, xModelInstance* model)
{
    _xBox* box = (_xBox*)bound;
    fn_80007738(bound, model);
    float* mat = ((PcModelView*)model)->Mat;
    box->upper.x += mat[12];
    box->upper.y += mat[13];
    box->upper.z += mat[14];
    box->lower.x += mat[12];
    box->lower.y += mat[13];
    box->lower.z += mat[14];
}
