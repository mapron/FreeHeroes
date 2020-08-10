/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#include "MediaConverter.hpp"

#include "FsUtilsQt.hpp"

#include <QProcess>
#include <QEventLoop>

namespace FreeHeroes::Conversion {

bool executeFFMpeg(QStringList args, QString * programOut = nullptr) {
    QProcess ffmpeg;
    ffmpeg.start("ffmpeg", args);
    if (!ffmpeg.waitForStarted())
        return false;
    if (programOut)
        *programOut = ffmpeg.program();

    ffmpeg.closeWriteChannel();

    if (!ffmpeg.waitForFinished())
        return false;

    if (ffmpeg.exitStatus() == QProcess::CrashExit)
        return false;

    // output:
    // Duration: 00:00:00.15
    // .tens of milliseconds, .15 = 150-159 milliseconds.

    const int rc = ffmpeg.exitCode();
    return rc == 0;
}

MediaConverter::MediaConverter()
{
    m_ffmpegFound = executeFFMpeg({"-version"}, &m_ffmpegBinary);
}

bool MediaConverter::prepareWav(const Core::std_path& source, const Core::std_path& dest)
{
    if (!m_ffmpegFound)
        return false;

    const auto success = executeFFMpeg({"-y", "-i", Gui::stdPath2QString(source), "-c:a", "pcm_s16le", Gui::stdPath2QString(dest)});
    return success;
}

bool MediaConverter::prepareVideo(const Core::std_path& source, const Core::std_path& dest)
{
    if (!m_ffmpegFound)
        return false;

//    const auto success = executeFFMpeg({"-y", "-i", Gui::stdPath2QString(source),
//                                        "-threads",  "1", "-c:v", "libx264", "-c:a", "aac",
//                                        Gui::stdPath2QString(dest)});

    const auto success = executeFFMpeg({"-y", "-i", Gui::stdPath2QString(source),
                                        "-threads",  "1",
                                        Gui::stdPath2QString(dest)});
    return success;
}


}
