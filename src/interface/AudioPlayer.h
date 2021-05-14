#pragma once

#include <napi.h>

#include <structs/PlayerCommand.h>

namespace CasperTech
{
    class AudioPlayerImpl;
}
namespace CasperTech::interface
{
    class AudioPlayer: public Napi::ObjectWrap<AudioPlayer>
    {
        public:
            static void Init(Napi::Env env, Napi::Object exports);

            explicit AudioPlayer(const Napi::CallbackInfo& info);
            ~AudioPlayer() override;

        private:
            Napi::Value load(const Napi::CallbackInfo& info);
            std::unique_ptr<CasperTech::AudioPlayerImpl> _audioPlayer;
    };
}
