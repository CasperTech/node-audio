#include <napi.h>
#include "interface/AudioPlayer.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports)
{
    CasperTech::interface::AudioPlayer::Init(env, exports);
    return exports;
}

NODE_API_MODULE(node_audio, InitAll)