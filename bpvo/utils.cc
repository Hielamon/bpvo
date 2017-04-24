/*
   This file is part of bpvo.

   bpvo is free software: you can redistribute it and/or modify
   it under the terms of the Lesser GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   bpvo is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   Lesser GNU General Public License for more details.

   You should have received a copy of the Lesser GNU General Public License
   along with bpvo.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * Contributor: halismai@cs.cmu.edu
 */
#if defined(WIN32) || defined(_WIN32)
#include <cctype>
#include <process.h>
#include <Windows.h>
#include <DbgHelp.h>
#include <direct.h>

#ifndef S_ISDIR
#define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)
#endif

#else
#include <unistd.h> // have this included first
#include <sys/time.h>
#include <sys/resource.h>
#include <execinfo.h>
#endif //linux

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iostream>
#include <thread>
#include <cstdarg>
#include <functional>

#include "bpvo/utils.h"
#include "bpvo/debug.h"

namespace bpvo {

#if defined(WIN32) || defined(_WIN32)
	int
		gettimeofday(struct timeval *tp, void *tzp)
	{
		time_t clock;
		struct tm tm;
		SYSTEMTIME wtm;
		GetLocalTime(&wtm);
		tm.tm_year = wtm.wYear - 1900;
		tm.tm_mon = wtm.wMonth - 1;
		tm.tm_mday = wtm.wDay;
		tm.tm_hour = wtm.wHour;
		tm.tm_min = wtm.wMinute;
		tm.tm_sec = wtm.wSecond;
		tm.tm_isdst = -1;
		clock = mktime(&tm);
		tp->tv_sec = clock;
		tp->tv_usec = wtm.wMilliseconds * 1000;
		return (0);
	}
#endif

bool icompare(const std::string& a, const std::string& b)
{
#if defined(WIN32) || defined(_WIN32)
	return a.size() == b.size() ? !strnicmp(a.c_str(), b.c_str(), a.size()) : false;
#else
	return a.size() == b.size() ? !strncasecmp(a.c_str(), b.c_str(), a.size()) : false;
#endif //linux
}

struct NoCaseCmp {
  inline bool operator()(const unsigned char& c1,
                         const unsigned char& c2) const
  {
    return std::tolower(c1) < std::tolower(c2);
  }
}; // NoCaseCmp

bool CaseInsenstiveComparator::operator()(const std::string& a,
                                          const std::string& b) const
{
  return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
                                      NoCaseCmp());
}

template<> int str2num<int>(const std::string& s) { return std::stoi(s); }

template <> double str2num<double>(const std::string& s) { return std::stod(s); }

template <> float str2num<float>(const std::string& s) { return std::stof(s); }

template <> bool str2num<bool>(const std::string& s)
{
  if(icompare(s, "true")) {
    return true;
  } else if(icompare(s, "false")) {
    return false;
  } else {
    // try to parse a bool from int {0,1}
    int v = str2num<int>(s);
    if(v == 0)
      return false;
    else if(v == 1)
      return true;
    else
      throw std::invalid_argument("string is not a boolean");
  }
}

int roundUpTo(int n, int m)
{
  return m ? ( (n % m) ? n + m - (n % m) : n) : n;
}

using std::string;
using std::vector;

vector<string> splitstr(const std::string& str, char delim)
{
  std::vector<std::string> ret;
  std::stringstream ss(str);
  std::string token;
  while(std::getline(ss, token, delim))
    ret.push_back(token);

  return ret;
}

string datetime()
{
  auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

  char buf[128];
  std::strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Z %Y", std::localtime(&tt));
  return std::string(buf);
}


double GetWallClockInSeconds()
{
  return wallclock();
}

uint64_t UnixTimestampSeconds()
{
  return std::chrono::seconds(std::time(NULL)).count();
}

uint64_t UnixTimestampMilliSeconds()
{
  return static_cast<int>( 1000.0 * UnixTimestampSeconds() );
}

void Sleep(int32_t ms)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

string Format(const char* fmt, ...)
{
  vector<char> buf(1024);

  while(true) {
    va_list va;
    va_start(va, fmt);
    auto len = vsnprintf(buf.data(), buf.size(), fmt, va);
    va_end(va);

    if(len < 0 || len >= (int) buf.size()) {
#ifndef max
      buf.resize(std::max((int)(buf.size() << 1), len + 1));
#else
	  buf.resize(max((int)(buf.size() << 1), len + 1));
#endif
      continue;
    }

    return string(buf.data(), len);
  }
}

string GetBackTrace()
{
	std::string ret;
	void* buf[1024];

#if defined(WIN32) || defined(_WIN32)
  HANDLE process = GetCurrentProcess();
  SymInitialize(process, NULL, TRUE);
  WORD n = CaptureStackBackTrace(0, 1024, buf, NULL);
  SYMBOL_INFO *symbol = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
  symbol->MaxNameLen = 255;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  char tempStr[255];
  
  for (int i = 0; i < n; i++)
  {
	  SymFromAddr(process, (DWORD64)(buf[i]), 0, symbol);
	  sprintf(tempStr, "%i: %s - 0x%0X\n", n - i - 1, symbol->Name, symbol->Address);
	  ret += std::string(tempStr);
  }
#else
  int n = backtrace(buf, 1024);
  char** strings = backtrace_symbols(buf, n);
  if (!strings) {
	  perror("backtrace_symbols\n");
	  return "";
  }

  
  for (int i = 0; i < n; ++i)
	  ret += (std::string(strings[i]) + "\n");

  free(strings);
#endif //linux
  
  

  return ret;
}

string dateAsString()
{
  std::time_t t;
  std::time(&t);

  char buf[64];
  std::strftime(buf, 64, "%Y-%m-%d", std::localtime(&t));

  return std::string( buf );
}

string timeAsString()
{
  std::time_t t;
  std::time(&t);

  char buf[64];
  std::strftime(buf, 64, "%A, %d.%B %Y, %H:%M", std::localtime(&t));

  return std::string( buf );
}

string errno_string()
{
  char buf[128];
  strerror_s(buf, 128, errno);
  return string(buf);
}

double wallclock()
{
  struct timeval tv;
  if( -1 == gettimeofday( &tv, NULL ) ) {
    Warn("could not gettimeofday, error: '%s'\n", errno_string().c_str());
  }

  return static_cast<double>( tv.tv_sec )
      + static_cast<double>( tv.tv_usec ) / 1.0E6;
}

double cputime()
{
#if defined(_WIN32)
	/* Windows -------------------------------------------------- */
	FILETIME createTime;
	FILETIME exitTime;
	FILETIME kernelTime;
	FILETIME userTime;
	if (GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &kernelTime, &userTime) != -1)
	{
		SYSTEMTIME userSystemTime;
		if (FileTimeToSystemTime(&userTime, &userSystemTime) == -1) {
			Warn("could not cpu usage, error '%s'\n", errno_string().c_str());
		}
		return (double)userSystemTime.wHour * 3600.0 +
			(double)userSystemTime.wMinute * 60.0 +
			(double)userSystemTime.wSecond +
			(double)userSystemTime.wMilliseconds / 1000.0;
	}

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
	/* AIX, BSD, Cygwin, HP-UX, Linux, OSX, and Solaris --------- */

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
	/* Prefer high-res POSIX timers, when available. */
	{
		clockid_t id;
		struct timespec ts;
#if _POSIX_CPUTIME > 0
		/* Clock ids vary by OS.  Query the id, if possible. */
		if (clock_getcpuclockid(0, &id) == -1)
#endif
#if defined(CLOCK_PROCESS_CPUTIME_ID)
			/* Use known clock id for AIX, Linux, or Solaris. */
			id = CLOCK_PROCESS_CPUTIME_ID;
#elif defined(CLOCK_VIRTUAL)
			/* Use known clock id for BSD or HP-UX. */
			id = CLOCK_VIRTUAL;
#else
			id = (clockid_t)-1;
#endif
		if (id != (clockid_t)-1 && clock_gettime(id, &ts) != -1)
			return (double)ts.tv_sec +
			(double)ts.tv_nsec / 1000000000.0;
	}
#endif

#if defined(RUSAGE_SELF)
	{
		struct rusage rusage;
		if (getrusage(RUSAGE_SELF, &rusage) == -1) {
			Warn("could not cpu usage, error '%s'\n", errno_string().c_str());
		}
		return (double)rusage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec / 1000000.0;
	}
#endif

#if defined(_SC_CLK_TCK)
	{
		const double ticks = (double)sysconf(_SC_CLK_TCK);
		struct tms tms;
		if (times(&tms) != (clock_t)-1)
			return (double)tms.tms_utime / ticks;
	}
#endif

#if defined(CLOCKS_PER_SEC)
	{
		clock_t cl = clock();
		if (cl != (clock_t)-1)
			return (double)cl / (double)CLOCKS_PER_SEC;
	}
#endif

#endif

	return -1;
}

namespace fs {
string expand_tilde(string fn)
{
  if(fn.front() == '~') {
    string home = getenv("HOME");
    if(home.empty()) {
      std::cerr << "could not query $HOME\n";
      return fn;
    }

    // handle the case when name == '~' only
    return home + dirsep(home) + ((fn.length()==1) ? "" :
                                  fn.substr(1,std::string::npos));
  } else {
    return fn;
  }
}

string dirsep(string dname)
{
  return (dname.back() == '/') ? "" : "/";
}

string extension(string filename)
{
  auto i = filename.find_last_of(".");
  return (string::npos != i) ? filename.substr(i) : "";
}

bool exists(string path)
{
  struct stat buf;
  return (0 == stat(path.c_str(), &buf));
}

bool is_regular(string path)
{
  struct stat buf;
  return (0 == stat(path.c_str(), &buf)) ? S_ISREG(buf.st_mode) : false;
}

bool is_dir(string path)
{
  struct stat buf;
  return (0 == stat(path.c_str(), &buf)) ? S_ISDIR(buf.st_mode) : false;
}

bool try_make_dir(string dname, int mode = 0777)
{
#if defined(WIN32) || defined(_WIN32)
	return (0 == ::_mkdir(dname.c_str()));
#else
	return (0 == ::mkdir(dname.c_str(), mode));
#endif
}

string mkdir(string dname, bool try_unique, int max_tries)
{
  if(!try_unique) {
    return try_make_dir(dname.c_str()) ? dname : "";
  } else {
    auto buf_len = dname.size() + 64;
    char* buf = new char[buf_len];
    int n = 0;
    snprintf(buf, buf_len, "%s-%05d", dname.c_str(), n);

    string ret;
    while(++n < max_tries) {
      if(try_make_dir(string(buf))) {
        ret = string(buf);
        break;
      }
    }

    delete[] buf;
    return ret;
  }
}

}; // fs

}; // bpvo


