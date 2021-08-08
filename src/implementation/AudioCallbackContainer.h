#pragma once

#include <mutex>

namespace CasperTech
{
	class RtAudioStream;
	struct AudioCallbackContainer
	{
	public:
		std::mutex containerMutex;
		RtAudioStream* rtAudioStream;
	};
}