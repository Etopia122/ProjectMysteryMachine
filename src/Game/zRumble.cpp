enum _tagRumbleType
{
    eRumble_Off = 0,
    eRumble_VeryLightHi = 1,
    eRumble_VeryLight = 2,
    eRumble_LightHi = 3,
    eRumble_Light = 4,
    eRumble_MediumHi = 5,
    eRumble_Medium = 6,
    eRumble_HeavyHi = 7,
    eRumble_Heavy = 8,
    eRumble_VeryHeavyHi = 9,
    eRumble_VeryHeavy = 10,
    eRumble_Total = 11,
};

enum _tagSDRumbleType
{
    SDR_Digging = 0,
    SDR_Headbutt = 1,
    SDR_ButtbounceJump = 2,
    SDR_Buttbounce = 3,
    SDR_VillainCollideScare = 4,
    SDR_FleaScratch = 5,
    SDR_ScreechingStop = 6,
    SDR_GotScared = 7,
    SDR_WebDestroyed = 8,
    SDR_XploDestroyedObj = 9,
    SDR_DustDestroyedObj = 10,
    SDR_EventLight = 11,
    SDR_EventMedium = 12,
    SDR_EventHeavy = 13,
    SDR_Test = 14,
    SDR_FireworkBang = 15,
    SDR_Total = 16,
};

void xPadAddRumble(signed int idx, _tagRumbleType type, float time, signed int replace,
                   unsigned int fxflags);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zRumbleStart(signed int pad_id, _tagSDRumbleType rumble_type)
{
    switch (rumble_type)
    {
    case SDR_Headbutt:
        xPadAddRumble(pad_id, eRumble_MediumHi, 0.2f, 1, 0);
        break;
    case SDR_ButtbounceJump:
        xPadAddRumble(pad_id, eRumble_Medium, 0.15f, 1, 0);
        break;
    case SDR_Buttbounce:
        xPadAddRumble(pad_id, eRumble_Medium, 0.15f, 1, 0);
        break;
    case SDR_VillainCollideScare:
    case SDR_GotScared:
        xPadAddRumble(pad_id, eRumble_Heavy, 0.15f, 1, 0);
        break;
    case SDR_FleaScratch:
        xPadAddRumble(pad_id, eRumble_Light, 1.5f, 1, 1);
        break;
    case SDR_ScreechingStop:
        xPadAddRumble(pad_id, eRumble_VeryLight, 0.5f, 1, 1);
        break;
    case SDR_Digging:
        xPadAddRumble(pad_id, eRumble_Medium, 0.1f, 1, 0);
        xPadAddRumble(pad_id, eRumble_Off, 0.15f, 0, 0);
        xPadAddRumble(pad_id, eRumble_Medium, 0.1f, 0, 0);
        xPadAddRumble(pad_id, eRumble_Off, 0.15f, 0, 0);
        xPadAddRumble(pad_id, eRumble_Medium, 0.1f, 0, 1);
        break;
    case SDR_WebDestroyed:
        xPadAddRumble(pad_id, eRumble_VeryLight, 0.1f, 1, 0);
        break;
    case SDR_XploDestroyedObj:
        xPadAddRumble(pad_id, eRumble_Medium, 0.1f, 1, 0);
        xPadAddRumble(pad_id, eRumble_Heavy, 0.1f, 0, 1);
        break;
    case SDR_DustDestroyedObj:
        xPadAddRumble(pad_id, eRumble_Light, 0.05f, 1, 0);
        xPadAddRumble(pad_id, eRumble_Medium, 0.15f, 0, 1);
        break;
    case SDR_Test:
        xPadAddRumble(pad_id, eRumble_LightHi, 0.5f, 1, 1);
        break;
    case SDR_EventLight:
        xPadAddRumble(pad_id, eRumble_Light, 0.35f, 1, 1);
        break;
    case SDR_EventMedium:
        xPadAddRumble(pad_id, eRumble_Medium, 0.35f, 1, 1);
        break;
    case SDR_EventHeavy:
        xPadAddRumble(pad_id, eRumble_Heavy, 0.35f, 1, 1);
        break;
    case SDR_FireworkBang:
        break;
    }
}

void zRumbleStartDistance(signed int pad_id, float real_dist, float max_dist)
{
    float mag;

    if (real_dist < 0.0f)
    {
        real_dist = 0.0f;
    }
    else if (real_dist > max_dist)
    {
        real_dist = max_dist;
    }

    mag = 0.35f * (1.0f - real_dist / max_dist);
    if (mag > 0.0f)
    {
        xPadAddRumble(pad_id, eRumble_Medium, mag, 1, 1);
    }
}
