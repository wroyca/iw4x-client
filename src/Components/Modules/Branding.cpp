#include <STDInclude.hpp>

namespace Components
{
	Dvar::Var Branding::CGDrawVersion;
	Dvar::Var Branding::CGDrawVersionX;
	Dvar::Var Branding::CGDrawVersionY;
	Game::dvar_t** Branding::Version = reinterpret_cast<Game::dvar_t**>(0x1AD7930);

#ifdef _DEBUG
	constexpr auto* BUILD_TYPE = "IW4x_DEV MP";
#else
	constexpr auto* BUILD_TYPE = "IW4x MP";
#endif

	void Branding::CG_DrawVersion()
	{
		// Default values
		constexpr auto fontScale = 0.25f;
		constexpr auto maxChars = std::numeric_limits<int>::max();
		// Default colours
		constexpr Game::vec4_t shadowColor = {0.0f, 0.0f, 0.0f, 0.69f};
		constexpr Game::vec4_t color = {0.4f, 0.69f, 1.0f, 0.69f};

		auto* const placement = Game::ScrPlace_GetUnsafeFullPlacement();
		auto* const font = Game::UI_GetFontHandle(placement, 0, 0.583f);

		const auto width = Game::UI_TextWidth((*Version)->current.string, 0, font, fontScale);
		const auto height = Game::UI_TextHeight(font, fontScale);

		Game::UI_DrawText(placement, (*Version)->current.string, maxChars, font, 1.0f - (CGDrawVersionX.get<float>() + static_cast<float>(width)),
			1.0f - (CGDrawVersionY.get<float>() + static_cast<float>(height)), 3, 3, fontScale, shadowColor, 0);
		Game::UI_DrawText(placement, (*Version)->current.string, maxChars, font, (0.0f - static_cast<float>(width)) - CGDrawVersionX.get<float>(),
			(0.0f - static_cast<float>(height)) - CGDrawVersionY.get<float>(), 3, 3, fontScale, color, 0);
	}

	void Branding::CG_DrawVersion_Hk(int localClientNum)
	{
		Utils::Hook::Call<void(int)>(0x4EFF80)(localClientNum);

		if (Branding::CGDrawVersion.get<bool>())
		{
			Branding::CG_DrawVersion();
		}
	}

	const char* Branding::GetBuildNumber()
	{
		return SHORTVERSION " latest " __DATE__ " " __TIME__;
	}

	const char* Branding::GetVersionString()
	{
		// IW4x is technically a beta
		const auto* result = Utils::String::VA("%s %s build %s %s",
			BUILD_TYPE, "(Beta)", Branding::GetBuildNumber(), reinterpret_cast<const char*>(0x7170A0));

		return result;
	}

	void Branding::Dvar_SetVersionString(const Game::dvar_t* dvar, [[maybe_unused]] const char* value)
	{
		const auto* result = Branding::GetVersionString();
		Utils::Hook::Call<void(const Game::dvar_t*, const char*)>(0x4A9580)(dvar, result);
	}

	// Branding this might be a good idea in case this LSP logging ever gets turned on for some reason
	void Branding::MSG_WriteVersionStringHeader(Game::msg_t* msg, [[maybe_unused]] const char* string)
	{
		const auto* result = Branding::GetVersionString();
		Utils::Hook::Call<void(Game::msg_t*, const char*)>(0x463820)(msg, result);
	}

	Game::dvar_t* Branding::Dvar_RegisterUIBuildLocation(const char* dvarName, [[maybe_unused]] float x,
		[[maybe_unused]] float y, float min, float max, [[maybe_unused]] int flags, const char* description)
	{
		return Game::Dvar_RegisterVec2(dvarName, -60.0f,
			474.0f, min, max, Game::DVAR_ROM, description);
	}

	void Branding::RegisterBrandingDvars()
	{
#ifdef _DEBUG
		constexpr auto value = true;
#else
		constexpr auto value = false;
#endif
		Branding::CGDrawVersion = Dvar::Register<bool>("cg_drawVersion", value,
			Game::DVAR_NONE, "Draw the game version");
		Branding::CGDrawVersionX = Dvar::Register<float>("cg_drawVersionX", 10.0f,
			0.0f, 512.0f, Game::DVAR_NONE, "X offset for the version string");
		Branding::CGDrawVersionY = Dvar::Register<float>("cg_drawVersionY", 455.0f,
			0.0f, 512.0f, Game::DVAR_NONE, "Y offset for the version string");
	}

	Branding::Branding()
	{
		Branding::RegisterBrandingDvars();

		// UI version string
		Utils::Hook::Set<const char*>(0x43F73B, "IW4x: " VERSION);

		// Short version dvar
		Utils::Hook::Set<const char*>(0x60BD91, SHORTVERSION);

		// Com_Init_Try_Block_Function
		Utils::Hook::Set<const char*>(0x60BAF4, BUILD_TYPE);
		Utils::Hook::Set<const char*>(0x60BAEf, SHORTVERSION);
		Utils::Hook::Set<const char*>(0x60BAE5, __DATE__);

		// G_InitGame
		Utils::Hook::Set<const char*>(0x48EBA1, __DATE__);

		Utils::Hook(0x4B12B0, Branding::GetBuildNumber, HOOK_JUMP).install()->quick();

		// Version string color
		static Game::vec4_t buildLocColor = {1.0f, 1.0f, 1.0f, 0.8f};
		Utils::Hook::Set<float*>(0x43F710, buildLocColor);

		// Place ui version string to bottom right corner (ui_buildlocation)
		Utils::Hook(0x6310A0, Branding::Dvar_RegisterUIBuildLocation, HOOK_CALL).install()->quick(); // Dvar_RegisterVec2

		Utils::Hook(0x60BD81, Branding::Dvar_SetVersionString, HOOK_CALL).install()->quick();

		Utils::Hook(0x4DA842, Branding::MSG_WriteVersionStringHeader, HOOK_CALL).install()->quick();

		// Hook CG_DrawFullScreenDebugOverlays so we may render the version when it's appropriate
		Utils::Hook(0x5AC975, Branding::CG_DrawVersion_Hk, HOOK_CALL).install()->quick();
	}
}