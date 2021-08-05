#include <implementation/FFSource.h>

#include <memory>
#include <implementation/SampleRateConverter.h>
#include <implementation/RtAudioRenderer.h>
#include <implementation/AudioPlayerImpl.h>

class EventReceiver: public CasperTech::IAudioPlayerEventReceiver
{
        void onPlayerEvent(const std::shared_ptr<CasperTech::PlayerEvent>& event) override
        {

        }
};

int main()
{
    auto recv = new EventReceiver();
    auto player = std::make_shared<CasperTech::AudioPlayerImpl>(recv);

    player->load(R"(C:\Users\Tom\Downloads\test.wav)", [player](CommandResult result, const std::string& errorMessage)
    {
        player->play([player](CommandResult result, const std::string& errorMessage)
         {

         });
    });
    while(1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}