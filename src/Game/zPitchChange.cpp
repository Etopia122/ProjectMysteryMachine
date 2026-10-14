float iurand();

inline float zPitchAbs(float x)
{
    return __fabs(x);
}

// TODO: 80.9%, structure matches; the original assigns the 360.0f constant f31 and max
// f30 (ours: max f31, 360.0f f28).
// Picks a new random angle at least 10 degrees away (in magnitude) from the old one and
// returns it; *offset gets the matching fraction of max. range is unused here.
signed int zPitchChange(signed int angle, float* offset, signed int range, float max)
{
    signed int old_angle = angle;

    do
    {
        angle = 360.0f * iurand();
    } while (zPitchAbs(old_angle) >= zPitchAbs(angle) - 10.0f && zPitchAbs(old_angle) <= zPitchAbs(angle) + 10.0f);

    *offset = (signed int)(angle / 360.0f * max);
    return angle;
}
