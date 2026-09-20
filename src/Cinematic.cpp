#include "Cinematic.h"

void CinematicManager::Update(SimulationState& state, float dt) {
    if (state.playCinematic && state.keyframes.size() >= 2) {
        state.cinematicTime += (dt * state.cinematicSpeed);
        float totalDuration = state.keyframes.back().time;
        if (state.cinematicTime > totalDuration) {
            state.cinematicTime = totalDuration;
            state.playCinematic = false;
            state.forceCamPos = false;
        } else {
            size_t k1 = 0;
            for (size_t i = 0; i < state.keyframes.size() - 1; i++) {
                if (state.cinematicTime >= state.keyframes[i].time && state.cinematicTime <= state.keyframes[i+1].time) {
                    k1 = i; break;
                }
            }
            Keyframe& a = state.keyframes[k1];
            Keyframe& b = state.keyframes[k1+1];
            float t = (state.cinematicTime - a.time) / (b.time - a.time);
            t = t * t * (3.0f - 2.0f * t);
            
            auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
            
            state.spin = lerp(a.spin, b.spin, t);
            state.q = lerp(a.q, b.q, t);
            state.accretionRate = lerp(a.acc, b.acc, t);
            state.brightmut = lerp(a.bright, b.bright, t);
            state.darkmut = lerp(a.dark, b.dark, t);
            state.reddening = lerp(a.redden, b.redden, t);
            state.saturation = lerp(a.sat, b.sat, t);
            state.jetBrightmut = lerp(a.jetBright, b.jetBright, t);
            state.jetLength = lerp(a.jetLength, b.jetLength, t);
            state.jetWidth = lerp(a.jetWidth, b.jetWidth, t);
            state.outerRadiusRs = lerp(a.radius, b.radius, t);
            state.bhSize = lerp(a.bhSize, b.bhSize, t);
            state.diskRotSpeed = lerp(a.rot, b.rot, t);
            state.diskColor[0] = lerp(a.color[0], b.color[0], t);
            state.diskColor[1] = lerp(a.color[1], b.color[1], t);
            state.diskColor[2] = lerp(a.color[2], b.color[2], t);
            
            for(int i=0; i<3; i++) {
                state.overridePos[i] = lerp(a.pos[i], b.pos[i], t);
                state.overrideFwd[i] = lerp(a.fwd[i], b.fwd[i], t);
                state.overrideRight[i] = lerp(a.right[i], b.right[i], t);
                state.overrideUp[i] = lerp(a.up[i], b.up[i], t);
            }
        }
    }
}
