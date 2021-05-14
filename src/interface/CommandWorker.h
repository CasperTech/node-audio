#include <napi.h>

#include <structs/PlayerCommand.h>

#include <functional>
#include <utility>
#include <condition_variable>

namespace CasperTech
{
    class CommandWorker: public Napi::AsyncWorker
    {
        public:
            CommandWorker(const Napi::Env& env, const Napi::Promise::Deferred& deferred, std::function<void(const ResultCallback& callback)>  workCallback)
                    : Napi::AsyncWorker(env),
                      _deferred(deferred),
                      _workCallback(std::move(workCallback))
            {

            }

            void Execute() override;
            void OnOK() override;

        private:
            std::function<void(const ResultCallback& callback)> _workCallback;
            Napi::Promise::Deferred _deferred;
            std::mutex _waitMutex;
            std::condition_variable _waitCondition;
            CommandResult _result = CommandResult::None;
            std::string _errorMessage;
    };
}
