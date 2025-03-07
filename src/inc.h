#ifndef INC
#define INC
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <utility>
#include <memory>
#include <fstream>
#include <napi.h>
#include <uv.h>
#include <fpdfview.h>
#include <cpp/fpdf_scopers.h>
#include "platform_defs.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <cups/cups.h>
#endif
#endif