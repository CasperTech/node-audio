#include "AudioPlayer.h"

#include <interface/CommandWorker.h>

#include <implementation/AudioPlayerImpl.h>

#include <iostream>
#include <memory>

namespace CasperTech::interface
{
    void AudioPlayer::Init(Napi::Env env, Napi::Object exports)
    {
        Napi::Function func = DefineClass(env, "AudioPlayer", {
                InstanceMethod("load", &AudioPlayer::load)
        });

        auto* constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("AudioPlayer", func);
    }

    AudioPlayer::AudioPlayer(const Napi::CallbackInfo& info)
            : Napi::ObjectWrap<AudioPlayer>(info),
              _audioPlayer(std::make_unique<AudioPlayerImpl>())
    {

    }

    Napi::Value AudioPlayer::load(const Napi::CallbackInfo& info)
    {
        if(info.Length() <= 0 || !info[0].IsString())
        {
            throw Napi::Error::New(info.Env(), "Must supply a filename parameter");
        }

        auto fileName = info[0].As<Napi::String>().Utf8Value();
        auto env = info.Env();
        Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

        auto worker = new CommandWorker(info.Env(), deferred, [this, fileName](const ResultCallback& callback)
        {
            _audioPlayer->load(fileName, callback);
        });

        worker->Queue();
        return deferred.Promise();
    }

    AudioPlayer::~AudioPlayer()
    {
        std::cout << "Destructing";
    }
}