#ifndef ISND_H
#define ISND_H

class xSndVoiceInfo;

void iSndDIEDIEDIE();
void iSndLoadSounds(void* info);
void iSndSceneExit();
void iSndSuspendCD(unsigned int suspend);
void iSndWaitForDeadSounds();
void iSndStereo(unsigned int stereo);
unsigned int iSndPlay(xSndVoiceInfo* vp);
unsigned int iSndFindFreeVoice(unsigned int priority, unsigned int flags);
void iSndUpdate(float dt);
void iSndStop(unsigned int snd);
void iSndPause(unsigned int snd, unsigned int pause);
void* iSndLookup(unsigned int id);
unsigned int iSndIsPlaying(unsigned int assetID, unsigned int parid);
unsigned int iSndIsPlayingByID(unsigned int assetID);
void iSndInitSceneLoaded();
void iSndExit();
void iSndInit();

#endif
