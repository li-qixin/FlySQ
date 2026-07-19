#pragma once

class Mixer
{
public:
	virtual ~Mixer() = default;
	virtual bool mix(float *outputs, unsigned output_count) = 0;
};

