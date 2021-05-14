//
// Created by Tom on 14/05/2021.
//

#include "CommandWorker.h"

void CasperTech::CommandWorker::Execute()
{
    std::unique_lock<std::mutex> lk(_waitMutex);
    _workCallback([this](CommandResult result, const std::string& errorMessage)
    {
        _result = result;
        _errorMessage = errorMessage;
        _waitCondition.notify_all();
    });

    _waitCondition.wait(lk, [this]
    {
        return _result != CommandResult::None;
    });
}

void CasperTech::CommandWorker::OnOK()
{
    if (_result == CommandResult::Success)
    {
        _deferred.Resolve(Env().Undefined());
    }
    else
    {
        auto err = Napi::Error::New(Env(), _errorMessage);
        _deferred.Reject(err.Value());
    }
}
