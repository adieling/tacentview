// Profile.cpp
//
// The viewer profile enum.
//
// Copyright (c) 2022-2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tStandard.h>
#include <Foundation/tFundamentals.h>
#include "Profile.h"
namespace Viewer
{


const char* ProfileNames[] =
{
	"Main",
	"Basic",
	"Kiosk",
	"Alt"
};
tStaticAssert(tNumElements(ProfileNames) == int(Profile::NumProfiles));


const char* ProfileNamesLong[] =
{
	"Main Profile",
	"Basic Profile",
	"Kiosk Profile",
	"Alt Profile"
};
tStaticAssert(tNumElements(ProfileNamesLong) == int(Profile::NumProfiles));


}
