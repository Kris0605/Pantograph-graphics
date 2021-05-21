#pragma once

#include "resource.h"
#include "Graphics.h"
#include "Drawables.h"

extern bool ExitSimu;
extern int SimIndex;


void SelectComPort();
void fillComboBoxSim();
void enableWindow();
void disableWindow();
void EnableCloseButton(const HWND hwnd, const BOOL bState);
