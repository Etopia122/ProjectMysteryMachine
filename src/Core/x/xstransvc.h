#ifndef XSTRANSVC_H
#define XSTRANSVC_H

class st_xST_XASSETINFO
{
public:
    unsigned int aid;
    unsigned int sector;
    unsigned int plus_offset;
    unsigned int size;
    void* mempos;
};

signed int xSTGetAssetInfoByType(unsigned int type, signed int idx, st_xST_XASSETINFO* ainfo);
signed int xSTGetAssetInfo(unsigned int aid, st_xST_XASSETINFO* ainfo);
void* xSTFindAssetByType(unsigned int type, signed int idx, unsigned int* size);
signed int xSTAssetCountByType(unsigned int type);
void* xSTFindAsset(unsigned int aid, unsigned int* size);
char* xSTAssetName(unsigned int aid);
char* xST_xAssetID_HIPFullPath(unsigned int aid);

#endif
