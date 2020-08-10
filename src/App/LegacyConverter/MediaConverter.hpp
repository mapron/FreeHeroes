/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QString>

#include "FsUtils.hpp"

namespace FreeHeroes::Conversion {

class MediaConverter {

public:
    MediaConverter();

    bool ffmpegFound() const { return m_ffmpegFound; }
    QString ffmpegBinary() const { return m_ffmpegBinary; }

    bool prepareWav(const Core::std_path & source, const Core::std_path & dest);
    bool prepareVideo(const Core::std_path & source, const Core::std_path & dest);

private:
    bool m_ffmpegFound = false;
    QString m_ffmpegBinary;
};

}
