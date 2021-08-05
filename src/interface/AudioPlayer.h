#pragma once

#include <napi.h>

#include <enums/PlaybackEvent.h>
#include <interfaces/IAudioPlayerEventReceiver.h>
#include <structs/events/CommandEvent.h>

#include <mutex>

namespace CasperTech
{
    class AudioPlayerImpl;
}
namespace CasperTech::interface
{
    class AudioPlayer: public Napi::ObjectWrap<AudioPlayer>, public IAudioPlayerEventReceiver
    {
        public:
            static void Init(Napi::Env env, Napi::Object exports);
            void onPlayerEvent(const std::shared_ptr<PlayerEvent>& event) override;

            explicit AudioPlayer(const Napi::CallbackInfo& info);
            ~AudioPlayer() override;

        private:
            void sendStatus(PlaybackEvent status, const std::string& message);
            Napi::Value load(const Napi::CallbackInfo& info);
            Napi::Value play(const Napi::CallbackInfo& info);
            Napi::Value stop(const Napi::CallbackInfo& info);
            Napi::Value seek(const Napi::CallbackInfo& info);
            Napi::Value pause(const Napi::CallbackInfo& info);
            Napi::Value setEventCallback(const Napi::CallbackInfo& info);
            std::shared_ptr<CasperTech::AudioPlayerImpl> _audioPlayer;
            std::mutex _statusCallbackMutex;
            Napi::ThreadSafeFunction _statusCallback;
            Napi::FunctionReference _statusCallbackRef;
    };
}
