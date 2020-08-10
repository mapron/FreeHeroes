/*
 * Copyright (C) 2020 Smirnov Valdimir / mapron1@gmail.com
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 */
#pragma once

#include <QDialog>

namespace FreeHeroes {
namespace Sound {
class IMusicBox;
}
class SoundTestWidget : public QDialog
{
public:
    SoundTestWidget(Sound::IMusicBox & musicBox);

};

}
