#include"Watercolor.hlsli"
#include"MathUtils.hlsli"

#define _NUM_THREADS_X_ 8
#define _NUM_THREADS_Y_ 8



// * For now we are simply applying noise to create changing pigment concentration.

[numthreads(_NUM_THREADS_X_, _NUM_THREADS_Y_, 1)]
void main(uint3 tid_ : SV_DispatchThreadID) {
	const uint2 coord = tid_.xy;
	const float2 uv = Watercolor::TexelSize * coord;
	
	Watercolor::Output::Simulation::Pigment[coord] =
		Watercolor::Input::Noise.SampleLevel(BilinearClamp, uv * 0.25f, 0.0f);
}