#include "STDInclude.hpp"

#define NEWS_MOTD_DEFUALT "Welcome to IW4x Multiplayer!"

namespace Components
{
	std::thread News::Thread;

	bool News::UnitTest()
	{
		bool result = true;

		if (News::Thread.joinable())
		{
			Logger::Print("Awaiting thread termination...\n");
			News::Thread.join();

			if (!strlen(Localization::Get("MPUI_CHANGELOG_TEXT")) || Localization::Get("MPUI_CHANGELOG_TEXT") == "Loading..."s)
			{
				Logger::Print("Failed to fetch changelog!\n");
				result = false;
			}
			else
			{
				Logger::Print("Successfully fetched changelog.\n");
			}

			if (!strcmp(Localization::Get("MPUI_MOTD_TEXT"), NEWS_MOTD_DEFUALT))
			{
				Logger::Print("Failed to fetch motd!\n");
				result = false;
			}
			else
			{
				Logger::Print("Successfully fetched motd.\n");
			}
		}

		return result;
	}

	News::News()
	{
		Localization::Set("MPUI_CHANGELOG_TEXT", "Loading...");
		Localization::Set("MPUI_MOTD_TEXT", NEWS_MOTD_DEFUALT);

		News::Thread = std::thread([] ()
		{
			Localization::Set("MPUI_CHANGELOG_TEXT", Utils::WebIO("IW4x", "https://iw4xcachep26muba.onion.to/iw4/changelog.txt").SetTimeout(5000)->Get().data());

			std::string data = Utils::WebIO("IW4x", "https://iw4xcachep26muba.onion.to/iw4/motd.txt").SetTimeout(5000)->Get();

			if (!data.empty())
			{
				Localization::Set("MPUI_MOTD_TEXT", data.data());
			}

			// TODO: Implement update checks here!
		});
	}

	News::~News()
	{
		if (News::Thread.joinable())
		{
			News::Thread.join();
		}
	}
}
