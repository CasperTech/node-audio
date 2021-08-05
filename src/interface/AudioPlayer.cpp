#include "AudioPlayer.h"

#include <interface/CommandWorker.h>

#include <implementation/AudioPlayerImpl.h>
#include <implementation/ScopedNodeRef.h>

#include <memory>
#include <structs/events/PlaybackErrorEvent.h>

namespace CasperTech::interface
{
    void AudioPlayer::Init(Napi::Env env, Napi::Object exports)
    {
        Napi::Function func = DefineClass(env, "AudioPlayer", {
                InstanceMethod("load", &AudioPlayer::load),
                InstanceMethod("play", &AudioPlayer::play),
                InstanceMethod("stop", &AudioPlayer::stop),
                InstanceMethod("seek", &AudioPlayer::seek),
                InstanceMethod("pause", &AudioPlayer::pause),
                InstanceMethod("setEventCallback", &AudioPlayer::setEventCallback)
        });

        auto* constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        constructor->SuppressDestruct();
        env.SetInstanceData(constructor);

        exports.Set("AudioPlayer", func);
    }

    AudioPlayer::AudioPlayer(const Napi::CallbackInfo& info)
            : Napi::ObjectWrap<AudioPlayer>(info),
              _audioPlayer(std::make_shared<AudioPlayerImpl>(this))
    {

    }

    Napi::Value AudioPlayer::play(const Napi::CallbackInfo& info)
    {
        auto env = info.Env();
        Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

        auto worker = new CommandWorker(info.Env(), deferred, [this](const ResultCallback& callback)
        {
            _audioPlayer->play(callback);
        });

        worker->Queue();
        return deferred.Promise();
    }

    Napi::Value AudioPlayer::stop(const Napi::CallbackInfo& info)
    {
        auto env = info.Env();
        Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

        auto worker = new CommandWorker(info.Env(), deferred, [this](const ResultCallback& callback)
        {
            _audioPlayer->stop(callback);
        });

        worker->Queue();
        return deferred.Promise();
    }

    Napi::Value AudioPlayer::seek(const Napi::CallbackInfo& info)
    {
        auto env = info.Env();
        Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
        if(info.Length() <= 0 || !info[0].IsNumber())
        {
            throw Napi::Error::New(env, "Must supply a seek time in milliseconds");
        }
        int64_t ms = info[0].As<Napi::Number>().DoubleValue();
        auto worker = new CommandWorker(info.Env(), deferred, [this, ms](const ResultCallback& callback)
        {
            _audioPlayer->seek(callback, ms);
        });

        worker->Queue();
        return deferred.Promise();
    }

    Napi::Value AudioPlayer::pause(const Napi::CallbackInfo& info)
    {
        auto env = info.Env();
        Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

        auto worker = new CommandWorker(info.Env(), deferred, [this](const ResultCallback& callback)
        {
            _audioPlayer->pause(callback);
        });

        worker->Queue();
        return deferred.Promise();
    }

    Napi::Value AudioPlayer::load(const Napi::CallbackInfo& info)
    {
        auto env = info.Env();
        if(info.Length() <= 0 || !info[0].IsString())
        {
            throw Napi::Error::New(env, "Must supply a filename parameter");
        }

        auto fileName = info[0].As<Napi::String>().Utf8Value();
        Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

        auto worker = new CommandWorker(info.Env(), deferred, [this, fileName](const ResultCallback& callback)
        {
            _audioPlayer->load(fileName, callback);
            sendStatus(PlaybackEvent::Loaded, "");
        });

        worker->Queue();
        return deferred.Promise();
    }

    Napi::Value AudioPlayer::setEventCallback(const Napi::CallbackInfo& info)
    {
        auto env = info.Env();
        std::unique_lock<std::mutex> lk(_statusCallbackMutex);
        if(info.Length() <= 0 || !info[0].IsFunction())
        {
            if(_statusCallback)
            {
                _statusCallback.Release();
                _statusCallbackRef.Reset();
            }
            return env.Undefined();
        }

        if(_statusCallback)
        {
            _statusCallback.Release();
            _statusCallbackRef.Reset();
        }

        auto func = info[0].As<Napi::Function>();
        func.Set("StatusCallback", Value());
        _statusCallbackRef = Napi::Persistent(func);
        _statusCallback = Napi::ThreadSafeFunction::New(
                env,
                _statusCallbackRef.Value(),  // JavaScript function called asynchronously
                "Resource Name",         // Name
                0,                       // Unlimited queue
                1,                       // Only one thread will use this initially
                [](Napi::Env){}
        );

        return env.Undefined();
    }

    AudioPlayer::~AudioPlayer()
    {

    }

    void AudioPlayer::sendStatus(PlaybackEvent status, const std::string& message)
    {
        std::unique_lock<std::mutex> lk(_statusCallbackMutex);
        if(!_statusCallback)
        {
            return;
        }
        auto persistent = &_statusCallbackRef;
        auto cb = _statusCallback;
        lk.unlock();

        auto ref = std::make_shared<ScopedNodeRef<Napi::Function>>(persistent);
        cb.NonBlockingCall([ref, status, message](Napi::Env env, Napi::Function jsCallback)
        {
            // std::cout << "Callback create str" << std::endl;
            auto str = Napi::String::New(env, message.c_str());
            // std::cout << "Callback status " << static_cast<int>(status) << " message " << message << std::endl;
            jsCallback.Call({
                Napi::Number::New(env, static_cast<unsigned int>(status)),
                str
            });
            // std::cout << "Callback done" << std::endl;
        });
    }

    void AudioPlayer::onPlayerEvent(const std::shared_ptr<PlayerEvent>& event)
    {
        if(!_statusCallback)
        {
            return;
        }

        switch(event->eventType)
        {
            case EventType::Command:
                return;
            case EventType::PlaybackFinished:
            {
                sendStatus(PlaybackEvent::Finished, "");
                break;
            }
            case EventType::PlaybackError:
            {
                auto evt = std::static_pointer_cast<PlaybackErrorEvent>(event);
                sendStatus(PlaybackEvent::Error, evt->msg);
                break;
            }
            case EventType::Playing:
            {
                sendStatus(PlaybackEvent::Playing, "");
                break;
            }
        }
    }
}