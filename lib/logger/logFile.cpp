#include "lynx/logger/LogFile.h"

#include <cassert>
#include <mutex>
#include <unistd.h>

#include <fmt/format.h>

namespace lynx {

LogFile::LogFile(const std::string &Basename, off_t RollSize, bool ThreadSafe,
                 int FlushInterval, int CheckEveryN)
    : Basename(Basename), RollSize(RollSize), FlushInterval(FlushInterval),
      CheckEveryN(CheckEveryN), Count(0),
      Mutex(ThreadSafe ? new std::mutex : nullptr), StartOfPeriod(0),
      LastRoll(0), LastFlush(0) {
  assert(Basename.find('/') == std::string::npos);
  rollFile();
}

LogFile::~LogFile() = default;

void LogFile::append(const char *Logline, int Len) {
  if (Mutex) {
    std::lock_guard<std::mutex> Lock(*Mutex);
    appendUnlocked(Logline, Len);
  } else {
    appendUnlocked(Logline, Len);
  }
}

void LogFile::flush() {
  if (Mutex) {
    std::lock_guard<std::mutex> Lock(*Mutex);
    File->flush();
  } else {
    File->flush();
  }
}

bool LogFile::rollFile() {
  time_t Now = 0;
  std::string Filename = getLogFileName(Basename, &Now);
  time_t Start = Now / KRollPerSeconds * KRollPerSeconds;

  /// If the current time is greater than the last roll time,
  /// update the last roll time, last flush time, and start of period time.
  if (Now > LastRoll) {
    LastRoll = Now;
    LastFlush = Now;
    StartOfPeriod = Start;
    /// Create a new AppendFile object with the generated filename.
    File = std::make_unique<util::AppendFile>(Filename);
    return true;
  }
  return false;
}

void LogFile::appendUnlocked(const char *Logline, int Len) {
  File->append(Logline, Len);

  if (File->writtenBytes() > RollSize) {
    rollFile();
  } else {
    ++Count;
    if (Count >= CheckEveryN) {
      Count = 0;
      time_t Now = ::time(nullptr);
      time_t ThisPeriod = Now / KRollPerSeconds * KRollPerSeconds;
      if (ThisPeriod != StartOfPeriod) {
        rollFile();
      } else if (Now - LastFlush > FlushInterval) {
        LastFlush = Now;
        File->flush();
      }
    }
  }
}

std::string LogFile::getLogFileName(const std::string &Basename, time_t *Now) {
  char Timebuf[32];
  std::tm Tm;
  *Now = time(nullptr);
  localtime_r(Now, &Tm); /// Convert the current time to local time.
  strftime(Timebuf, sizeof(Timebuf), "%Y%m%d-%H%M%S", &Tm);

  char Buf[256];
  if (::gethostname(Buf, sizeof(Buf)) == 0) {
    // Filename += Buf;
  } else {
    strcpy(Buf, "unknownhost");
  }

  return fmt::format(
      "{}.{}.{}.{:d}.log",
      Basename,  /// (1). Copy the base name of the log file.
      Timebuf,   /// (2). Format the current time as a string.
      Buf,       /// (3). Get the host name and append it to the filename.
      ::getpid() /// (4). Format the process ID as a string.
  );
  // return Filename;
}

} // namespace lynx
