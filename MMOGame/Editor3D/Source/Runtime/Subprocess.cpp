#include "Subprocess.h"

#include <cstring>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace MMO {

#ifdef _WIN32

	namespace {

		// Quote and escape a single argument for a Windows command line per the rules
		// CommandLineToArgvW uses to parse them back.
		std::wstring QuoteArgW(const std::wstring& a)
		{
			if (!a.empty() && a.find_first_of(L" \t\"") == std::wstring::npos)
			{
				return a;
			}
			std::wstring out;
			out.push_back(L'"');
			int backslashes = 0;
			for (wchar_t c : a)
			{
				if (c == L'\\')
				{
					++backslashes;
				}
				else if (c == L'"')
				{
					out.append(backslashes * 2 + 1, L'\\');
					out.push_back(L'"');
					backslashes = 0;
				}
				else
				{
					out.append(backslashes, L'\\');
					backslashes = 0;
					out.push_back(c);
				}
			}
			out.append(backslashes * 2, L'\\');
			out.push_back(L'"');
			return out;
		}

		std::wstring Widen(const std::string& s)
		{
			if (s.empty())
				return {};
			int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
			std::wstring out(n, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), out.data(), n);
			return out;
		}

	} // namespace

	bool Subprocess::Start(const std::filesystem::path& exe,
						   const std::vector<std::string>& args,
						   const std::vector<std::string>& envOverrides,
						   const std::filesystem::path& cwd)
	{
		Reset();

		std::wstring cmdLine = QuoteArgW(exe.wstring());
		for (const auto& a : args)
		{
			cmdLine.push_back(L' ');
			cmdLine += QuoteArgW(Widen(a));
		}

		// Build env block: copy current env, then apply overrides. Format is a
		// double-NUL terminated list of NUL-separated NAME=value strings.
		std::wstring envBlock;
		LPWCH currentEnv = GetEnvironmentStringsW();
		for (LPWCH p = currentEnv; *p;)
		{
			envBlock.append(p);
			envBlock.push_back(L'\0');
			p += wcslen(p) + 1;
		}
		FreeEnvironmentStringsW(currentEnv);
		for (const auto& e : envOverrides)
		{
			envBlock += Widen(e);
			envBlock.push_back(L'\0');
		}
		envBlock.push_back(L'\0');

		STARTUPINFOW si{};
		si.cb = sizeof(si);

		std::wstring wcwd = cwd.empty() ? std::wstring() : cwd.wstring();

		BOOL ok = CreateProcessW(
			nullptr,		  // application name (null = use cmdLine)
			cmdLine.data(),	  // command line (must be writable)
			nullptr, nullptr, // security attrs
			FALSE,			  // inherit handles
			CREATE_UNICODE_ENVIRONMENT,
			envBlock.data(),
			wcwd.empty() ? nullptr : wcwd.c_str(),
			&si, &m_Pi);

		if (!ok)
		{
			std::cerr << "[Subprocess] CreateProcessW failed: " << GetLastError() << std::endl;
			return false;
		}
		m_Started = true;
		return true;
	}

	bool Subprocess::IsRunning()
	{
		if (!m_Started || m_Exited)
			return false;
		DWORD status = WaitForSingleObject(m_Pi.hProcess, 0);
		if (status == WAIT_TIMEOUT)
			return true;
		DWORD code = 0;
		GetExitCodeProcess(m_Pi.hProcess, &code);
		m_ExitCode = (int)code;
		m_Exited = true;
		CloseHandle(m_Pi.hThread);
		CloseHandle(m_Pi.hProcess);
		m_Pi = {};
		return false;
	}

	void Subprocess::Stop()
	{
		if (!m_Started || m_Exited)
			return;
		TerminateProcess(m_Pi.hProcess, 1);
		WaitForSingleObject(m_Pi.hProcess, 2000);
		DWORD code = 0;
		GetExitCodeProcess(m_Pi.hProcess, &code);
		m_ExitCode = (int)code;
		m_Exited = true;
		CloseHandle(m_Pi.hThread);
		CloseHandle(m_Pi.hProcess);
		m_Pi = {};
	}

	void Subprocess::Reset()
	{
		if (m_Started && !m_Exited)
		{
			Stop();
		}
		m_Started = false;
		m_Exited = false;
		m_ExitCode = 0;
	}

	Subprocess::Subprocess(Subprocess&& other) noexcept
		: m_Pi(other.m_Pi),
		  m_Started(other.m_Started),
		  m_Exited(other.m_Exited),
		  m_ExitCode(other.m_ExitCode)
	{
		other.m_Pi = {};
		other.m_Started = false;
		other.m_Exited = false;
	}

	Subprocess& Subprocess::operator=(Subprocess&& other) noexcept
	{
		if (this != &other)
		{
			Reset();
			m_Pi = other.m_Pi;
			m_Started = other.m_Started;
			m_Exited = other.m_Exited;
			m_ExitCode = other.m_ExitCode;
			other.m_Pi = {};
			other.m_Started = false;
			other.m_Exited = false;
		}
		return *this;
	}

	Subprocess::~Subprocess() { Reset(); }

#else // POSIX

	bool Subprocess::Start(const std::filesystem::path& exe,
						   const std::vector<std::string>& args,
						   const std::vector<std::string>& envOverrides,
						   const std::filesystem::path& cwd)
	{
		Reset();

		pid_t pid = fork();
		if (pid < 0)
		{
			std::cerr << "[Subprocess] fork() failed: " << strerror(errno) << std::endl;
			return false;
		}

		if (pid == 0)
		{
			// Child
			for (const auto& e : envOverrides)
			{
				// setenv with overwrite=1; "NAME=value" → split.
				auto eq = e.find('=');
				if (eq != std::string::npos)
				{
					setenv(e.substr(0, eq).c_str(), e.substr(eq + 1).c_str(), 1);
				}
			}

			if (!cwd.empty())
			{
				if (chdir(cwd.c_str()) != 0)
				{
					std::cerr << "[Subprocess] child chdir(" << cwd.string()
							  << ") failed: " << strerror(errno) << std::endl;
					_exit(127);
				}
			}

			std::vector<std::string> argStorage;
			argStorage.reserve(args.size() + 1);
			argStorage.push_back(exe.string());
			for (const auto& a : args)
				argStorage.push_back(a);

			std::vector<char*> argv;
			argv.reserve(argStorage.size() + 1);
			for (auto& s : argStorage)
				argv.push_back(s.data());
			argv.push_back(nullptr);

			execvp(exe.c_str(), argv.data());
			std::cerr << "[Subprocess] execvp(" << exe.string() << ") failed: "
					  << strerror(errno) << std::endl;
			_exit(127);
		}

		// Parent
		m_Pid = pid;
		return true;
	}

	bool Subprocess::IsRunning()
	{
		if (m_Pid <= 0 || m_Exited)
			return false;
		int status = 0;
		pid_t r = waitpid(m_Pid, &status, WNOHANG);
		if (r == 0)
			return true;
		if (r == m_Pid)
		{
			m_Exited = true;
			if (WIFEXITED(status))
				m_ExitCode = WEXITSTATUS(status);
			else if (WIFSIGNALED(status))
				m_ExitCode = 128 + WTERMSIG(status);
			m_Pid = -1;
			return false;
		}
		if (errno == ECHILD)
		{
			m_Exited = true;
			m_Pid = -1;
		}
		return false;
	}

	void Subprocess::Stop()
	{
		if (m_Pid <= 0 || m_Exited)
			return;
		kill(m_Pid, SIGTERM);
		// Give the child up to 2s to exit cleanly.
		for (int i = 0; i < 20; ++i)
		{
			int status = 0;
			pid_t r = waitpid(m_Pid, &status, WNOHANG);
			if (r == m_Pid)
			{
				m_Exited = true;
				if (WIFEXITED(status))
					m_ExitCode = WEXITSTATUS(status);
				else if (WIFSIGNALED(status))
					m_ExitCode = 128 + WTERMSIG(status);
				m_Pid = -1;
				return;
			}
			usleep(100000); // 100ms
		}
		// Still alive — escalate.
		kill(m_Pid, SIGKILL);
		int status = 0;
		waitpid(m_Pid, &status, 0);
		m_Exited = true;
		m_ExitCode = 137;
		m_Pid = -1;
	}

	void Subprocess::Reset()
	{
		if (m_Pid > 0 && !m_Exited)
			Stop();
		m_Pid = -1;
		m_Exited = false;
		m_ExitCode = 0;
	}

	Subprocess::Subprocess(Subprocess&& other) noexcept
		: m_Pid(other.m_Pid),
		  m_Exited(other.m_Exited),
		  m_ExitCode(other.m_ExitCode)
	{
		other.m_Pid = -1;
		other.m_Exited = false;
	}

	Subprocess& Subprocess::operator=(Subprocess&& other) noexcept
	{
		if (this != &other)
		{
			Reset();
			m_Pid = other.m_Pid;
			m_Exited = other.m_Exited;
			m_ExitCode = other.m_ExitCode;
			other.m_Pid = -1;
			other.m_Exited = false;
		}
		return *this;
	}

	Subprocess::~Subprocess() { Reset(); }

#endif

} // namespace MMO
