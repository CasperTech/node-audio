#include <implementation/FFSource.h>

#include <memory>
#include <implementation/SampleRateConverter.h>
#include <implementation/RtAudioRenderer.h>
#include <implementation/AudioPlayerImpl.h>

#include <thread>
#include <mutex>
#include <condition_variable>

std::mutex waitForCommandMutex;
std::condition_variable waitForCommand;

class EventReceiver: public CasperTech::IAudioPlayerEventReceiver
{
        void onPlayerEvent(const std::shared_ptr<CasperTech::PlayerEvent>& event) override
        {
            if (event->eventType != EventType::Command)
            {
                std::cout << "Got event " << unsigned(event->eventType) << std::endl;
            }
        }
};

void commandDone(CommandResult result, const std::string& errorMessage)
{
    std::unique_lock<std::mutex> lk(waitForCommandMutex);
    waitForCommand.notify_one();
}

int main()
{
    std::unique_lock<std::mutex> wait(waitForCommandMutex);
    auto recv = new EventReceiver();
    auto player = std::make_shared<CasperTech::AudioPlayerImpl>(recv);
    std::cout << "Loading File" << std::endl;
    player->load(R"(C:\Projects\@caspertech\node-audio\media\piano.wav)", &commandDone);
    waitForCommand.wait(wait);

    std::cout << "Fileloaded. Seeking to 2 seconds" << std::endl;
    player->seek(2000, &commandDone);
    waitForCommand.wait(wait);

    std::cout << "Waiting for 2 seconds before playing" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    std::cout << "Playing for 2.0 seconds befor seeking back to 0" << std::endl;
    player->play(&commandDone);
    waitForCommand.wait(wait);

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    player->seek(0, &commandDone);
    waitForCommand.wait(wait);

    std::cout << "Waiting for 5 seconds then seeking back to 0" << std::endl;;
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    player->seek(0, &commandDone);
    waitForCommand.wait(wait);

    std::cout << "Waiting for 5 seconds then setting volume to 0.1" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    player->setVolume(0.1f, &commandDone);
    waitForCommand.wait(wait);
    std::cout << "Done" << std::endl;
    while (1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
