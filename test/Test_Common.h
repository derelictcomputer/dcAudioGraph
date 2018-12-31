#pragma once

namespace dc
{
class AudioBuffer;
bool samplesEqual(float s0, float s1);
bool buffersEqual(dc::AudioBuffer& b0, dc::AudioBuffer& b1);
}
