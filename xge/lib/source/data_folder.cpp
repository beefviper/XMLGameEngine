// data_folder.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "data_folder.h"

#include <system_error>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <cstdint>
#endif

namespace xge
{
	std::filesystem::path programDirectory()
	{
#if defined(_WIN32)
		std::vector<wchar_t> buffer(MAX_PATH);
		for (;;)
		{
			const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
			if (length == 0)
			{
				return {};
			}

			// The path fits when the length is under the buffer size; a full
			// buffer means it was cut short, so try a larger one.
			if (length < buffer.size())
			{
				return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
			}

			buffer.resize(buffer.size() * 2);
		}
#elif defined(__APPLE__)
		std::uint32_t size = 0;
		_NSGetExecutablePath(nullptr, &size);
		std::vector<char> buffer(size);
		if (_NSGetExecutablePath(buffer.data(), &size) != 0)
		{
			return {};
		}

		std::error_code error;
		const auto resolved = std::filesystem::weakly_canonical(buffer.data(), error);
		return error ? std::filesystem::path() : resolved.parent_path();
#else
		std::error_code error;
		const auto program = std::filesystem::read_symlink("/proc/self/exe", error);
		return error ? std::filesystem::path() : program.parent_path();
#endif
	}

	std::filesystem::path findDataFolder(const std::filesystem::path& workingDirectory,
		const std::filesystem::path& programDirectory)
	{
		std::vector<std::filesystem::path> bases{ workingDirectory };
		if (!programDirectory.empty())
		{
			bases.push_back(programDirectory);
			bases.push_back(programDirectory / "..");
		}

		for (const auto& base : bases)
		{
			if (base.empty())
			{
				continue;
			}

			for (const auto& candidate : { base, base / "games" })
			{
				std::error_code error;
				if (!std::filesystem::is_directory(candidate / "descriptions", error) ||
					!std::filesystem::is_directory(candidate / "assets", error))
				{
					continue;
				}

				const auto clean = std::filesystem::weakly_canonical(candidate, error);
				return error ? candidate.lexically_normal() : clean;
			}
		}

		return {};
	}

	std::filesystem::path enterDataFolder()
	{
		std::error_code error;
		const auto folder = findDataFolder(std::filesystem::current_path(error), programDirectory());
		if (folder.empty())
		{
			return {};
		}

		std::filesystem::current_path(folder, error);
		return error ? std::filesystem::path() : folder;
	}

	std::filesystem::path gameFileGiven(const std::string& game)
	{
		std::filesystem::path given(game);

		if (!given.has_extension())
		{
			given += ".xml";
		}

		return given;
	}

	std::filesystem::path findSchema(const std::filesystem::path& gameFile, const std::string& schemaLocation)
	{
		// A file that is not there sets the error code, so each check has its own.
		const auto isFile = [](const std::filesystem::path& file)
		{
			std::error_code missing;
			return std::filesystem::is_regular_file(file, missing);
		};

		const std::filesystem::path folder = gameFile.parent_path();
		const std::filesystem::path named = folder / schemaLocation;
		if (isFile(named))
		{
			return named;
		}

		const std::filesystem::path name = std::filesystem::path(schemaLocation).filename();
		std::error_code error;
		std::filesystem::path at = std::filesystem::absolute(folder.empty() ? std::filesystem::path(".") : folder, error).lexically_normal();
		while (!error && !name.empty())
		{
			if (isFile(at / name))
			{
				return at / name;
			}
			if (at == at.parent_path() || !at.has_relative_path())
			{
				break;
			}
			at = at.parent_path();
		}

		return named;
	}

	std::optional<std::filesystem::path> locateGameFile(const std::string& game,
		const std::filesystem::path& gamesDirectory)
	{
		const std::filesystem::path given = gameFileGiven(game);
		const std::filesystem::path fileName = given.filename();

		const std::filesystem::path candidates[] = {
			given,
			fileName,
			gamesDirectory / fileName,
		};

		for (const auto& candidate : candidates)
		{
			std::error_code error;
			if (std::filesystem::is_regular_file(candidate, error))
			{
				return candidate;
			}
		}

		return std::nullopt;
	}

	std::filesystem::path keptFileFor(const std::filesystem::path& gameFile)
	{
		return std::filesystem::path{ "saves" } / gameFile.stem().concat(".xml");
	}
}
