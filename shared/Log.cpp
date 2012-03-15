#include "Log.h"
#include <stdio.h>
#include <QDateTime>
#include <QMutex>
#include <QCoreApplication>

static int sLevel = 0;
static FILE *sFile = 0;
static QByteArray sLogFile;

static void log(int level, const char *format, va_list v)
{
    if (level > sLevel && !sFile)
        return;
    enum { Size = 16384 };
    char buf[Size];
    char *msg = buf;
    int n = vsnprintf(buf, Size, format, v);
    if (n == -1) {
        return;
    }
    if (n >= Size) {
        msg = new char[n + 1];
        n = vsnprintf(msg, n + 1, format, v);
    }
    const QByteArray now = QDateTime::currentDateTime().toString("dd/MM/yy hh:mm:ss").toLocal8Bit();
    static QMutex mutex;
    QMutexLocker lock(&mutex); // serialize
    static const char *names[] = { "Error: ", "Warning: ", "Debug: ", "Verbose: " };
    const char *name = level < static_cast<int>(sizeof(names)) / 4 ? names[level] : "";
    if (level <= sLevel)
        fprintf(stderr, "%s: %s%s\n", now.constData(), name, msg);
    if (sFile) {
        fprintf(sFile, "%s: %s%s\n", now.constData(), name, msg);
        fflush(sFile);
    }
    if (msg != buf)
        delete []msg;
}

void log(int level, const char *format, ...)
{
    va_list v;
    va_start(v, format);
    log(level, format, v);
    va_end(v);
}

void debug(const char *format, ...)
{
    va_list v;
    va_start(v, format);
    log(Debug, format, v);
    va_end(v);
}

void warning(const char *format, ...)
{
    va_list v;
    va_start(v, format);
    log(Warning, format, v);
    va_end(v);
}

void error(const char *format, ...)
{
    va_list v;
    va_start(v, format);
    log(Error, format, v);
    va_end(v);
}

int logLevel()
{
    return sLevel;
}

QByteArray logFile()
{
    return sLogFile;
}

static inline void removeLogFile()
{
    Q_ASSERT(sFile);
    fflush(sFile);
    fclose(sFile);
    sFile = 0;
}

bool testLog(int level)
{
    return level <= sLevel || sFile;
}

bool initLogging(int level, const QByteArray &file, unsigned flags)
{
    sLevel = level;
    if (!file.isEmpty()) {
        sFile = fopen(file.constData(), flags & Append ? "a" : "w");
        if (!sFile)
            return false;
        sLogFile = file;
        qAddPostRoutine(removeLogFile);
    }
    return true;
}

Log::Log(int level)
{
    mData = new Data(level);
}

Log::Log(const Log &other)
    : mData(other.mData)
{
}

Log &Log::operator=(const Log &other)
{
    mData = other.mData;
    return *this;
}
