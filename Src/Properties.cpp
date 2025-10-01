// Properties.cpp
//
// Image properties display and editor window.
//
// Copyright (c) 2019-2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include "imgui.h"
#include "imgui_internal.h"
#include "Properties.h"
#include "Config.h"
#include "Image.h"
#include "TacentView.h"
#include "GuiUtil.h"
using namespace tMath;
using namespace tImage;

// Globals from TacentView.cpp controlling unified preview behaviour.
namespace Viewer { extern bool gShowAllMipsUnified; extern bool gShowAllArrayLayers; extern bool gShowLayerMipMatrix; }


namespace Viewer
{
	// Returns true if any UI was drawn. Currently called for DDS, KTX, KTX2, and PVR files.
	bool DoMultiSurface(float itemWidth);
}


bool Viewer::DoMultiSurface(float itemWidth)
{
	bool anyDraw = false;
	// Unified handling: For texture arrays we do not enable AltPicture; we toggle a global flag (declared in TacentView.cpp).

	bool isTextureArray = (CurrImage->GetMultiFrameType() == Image::MultiFrameType::TextureArray);
	bool altMipmapsPicAvail = CurrImage->IsAltMipmapsPictureAvail() && !CropMode; // For non-arrays (legacy side-by-side).
	bool altMipmapsPicEnabl = altMipmapsPicAvail && CurrImage->IsAltPictureEnabled();
	if (isTextureArray && (CurrImage->GetNumMipLevels() > 1))
	{
		if (ImGui::Checkbox("Display All Mipmaps", &gShowAllMipsUnified))
		{
			// When switching off unified view ensure AltPicture disabled if it was somehow on.
			if (!gShowAllMipsUnified && CurrImage->IsAltPictureEnabled())
			{
				CurrImage->EnableAltPicture(false);
				CurrImage->Bind();
			}
		}
		Gutil::ToolTip("Show a separate Mip Chain window with all mip levels of the current array layer.");
		anyDraw = true;
	        if (CurrImage->GetNumArrayLayers() > 1)
	        {
	            if (ImGui::Checkbox("Display All Layers", &gShowAllArrayLayers)) { }
	            Gutil::ToolTip("Show a grid of all array layers am aktuellen Mip.");
	            if (ImGui::Checkbox("Layer/Mip Matrix", &gShowLayerMipMatrix)) { }
	            Gutil::ToolTip("Zeigt alle Layer *und* alle Mip Levels als Matrix.");
	        }
	}
	else if (altMipmapsPicAvail)
	{
		if (ImGui::Checkbox("Display All Mipmaps", &altMipmapsPicEnabl))
		{
			CurrImage->EnableAltPicture(altMipmapsPicEnabl);
			CurrImage->Bind();
		}
		Gutil::ToolTip("Display all mipmaps in a single side-by-side image.");
		anyDraw = true;
	}

	bool altCubemapPicAvail = CurrImage->IsAltCubemapPictureAvail() && !CropMode;
	bool altCubemapPicEnabl = altCubemapPicAvail && CurrImage->IsAltPictureEnabled();
	if (altCubemapPicAvail)
	{
		if (ImGui::Checkbox("Display As Cubemap", &altCubemapPicEnabl))
		{
			CurrImage->EnableAltPicture(altCubemapPicEnabl);
			CurrImage->Bind();
		}
		Gutil::ToolTip("Display all cubemap sides in a T-layout.");
		anyDraw = true;
	}

	int numTextures = CurrImage->GetNumFrames();
	if ((numTextures >= 2) && (!CurrImage->IsAltPictureEnabled()))
	{
		if (altCubemapPicAvail)
		{
			int numCubeSurfs = tFaceIndex_NumFaces;
			int numCubeMips = numTextures / numCubeSurfs;
			int oneBasedSurfNum = CurrImage->FrameNum/numCubeMips + 1;
			int oneBasedMipNum = CurrImage->FrameNum - ((oneBasedSurfNum-1)*numCubeMips) + 1;

			tString surfNumText;
			tsPrintf(surfNumText, "Cube Side (%d)", numCubeSurfs);
			ImGui::SetNextItemWidth(itemWidth);
			if (ImGui::InputInt(surfNumText.Chr(), &oneBasedSurfNum))
			{
				tMath::tiClamp(oneBasedSurfNum, 1, numCubeSurfs);
				CurrImage->FrameNum = (oneBasedSurfNum-1)*numCubeMips + (oneBasedMipNum-1);
			}
			ImGui::SameLine(); Gutil::HelpMark
			(
				"Cubemap side to display. Cubemaps use a left-handed ordering\n"
				"with +Z facing forward and +Y up. Sides are shown in the order\n"
				"+Z,-Z,+X,-X,+Y,-Y. That is, front, back, right, left, top, bottom."
			);

			if (numCubeMips > 1)
			{
				tString mipNumText;
				tsPrintf(mipNumText, "Cube Mip (%d)", numCubeMips);
				ImGui::SetNextItemWidth(itemWidth);
				if (ImGui::InputInt(mipNumText.Chr(), &oneBasedMipNum))
				{
					tMath::tiClamp(oneBasedMipNum, 1, numCubeMips);
					CurrImage->FrameNum = (oneBasedSurfNum-1)*numCubeMips + (oneBasedMipNum-1);
				}
				ImGui::SameLine(); Gutil::HelpMark("Which cubemap mipmap to display.");
			}
		}
		else
		{
			tString imageNumText;
			tsPrintf(imageNumText, "%s (%d)", altMipmapsPicAvail ? "Mipmap" : "Texture", numTextures);
			int oneBasedTextureNum = CurrImage->FrameNum + 1;
			ImGui::SetNextItemWidth(itemWidth);
			if (ImGui::InputInt(imageNumText.Chr(), &oneBasedTextureNum))
			{
				CurrImage->FrameNum = oneBasedTextureNum - 1;
				tMath::tiClamp(CurrImage->FrameNum, 0, numTextures-1);
			}
			ImGui::SameLine(); Gutil::HelpMark("Which mipmap or texture to display.");
		}
		anyDraw = true;
	}

	return anyDraw;
}


void Viewer::ShowPropertiesWindow(bool* popen)
{
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar;

	// We specify a default position/size in case there's no data in the .ini file. Typically this isn't required! We only
	// do it to make the Demo applications a little more welcoming.
	tVector2 windowPos = Gutil::GetDialogOrigin(Gutil::DialogID::Properties);
	ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);

	Config::ProfileData& profile = Config::GetProfileData();
	float nextWinWidth = Gutil::GetUIParamScaled(238.0f, 2.5f);
	ImGui::SetNextWindowSize(tVector2(nextWinWidth, -1.0f), ImGuiCond_Always);

	if (!ImGui::Begin("Properties", popen, windowFlags))
	{
		ImGui::End();
		return;
	}

	if (!CurrImage)
	{
		ImGui::Text("No Images in Folder");
		ImGui::End();
		return;
	}
	else if (CurrImage && !CurrImage->IsLoaded())
	{
		ImGui::Text("Image Failed to Load");
		ImGui::End();
		return;
	}

	float itemWidth = Gutil::GetUIParamScaled(110.0f, 2.5f);
	float imageSize = Gutil::GetUIParamScaled(18.0f, 2.5f);
	tVector2 imgButtonSize(imageSize, imageSize);

	bool fileTypeSectionDisplayed = false;
	switch (CurrImage->Filetype)
	{
		case tSystem::tFileType::DDS:
		{
			bool anyUIDisplayed = false;
			int numTextures = CurrImage->GetNumFrames();
			bool reloadChanges = false;

			anyUIDisplayed |= DoMultiSurface(itemWidth);

			if (tIsETCFormat(CurrImage->Info.SrcPixelFormat))
			{
				if (ImGui::CheckboxFlags("SwizzleBGRToRGB", &CurrImage->LoadParams_DDS.Flags, tImageDDS::LoadFlag_SwizzleBGR2RGB))
					reloadChanges = true;
				anyUIDisplayed = true;
			}

			// If we're here show options when have 1 or more frames.
			bool altEnabled = CurrImage->IsAltPictureEnabled();

			// Gamma correction. First read current setting and put it in an int.
			int gammaMode = 0;
			if (CurrImage->LoadParams_DDS.Flags & tImageDDS::LoadFlag_GammaCompression)
				gammaMode = 1;
			if (CurrImage->LoadParams_DDS.Flags & tImageDDS::LoadFlag_SRGBCompression)
				gammaMode = 2;
			if (CurrImage->LoadParams_DDS.Flags & tImageDDS::LoadFlag_AutoGamma)
				gammaMode = 3;
			const char* gammaCorrectItems[] = { "None", "Gamma", "sRGB", "Auto" };
			ImGui::SetNextItemWidth(itemWidth);
			if (ImGui::Combo("Gamma Corr", &gammaMode, gammaCorrectItems, tNumElements(gammaCorrectItems)))
			{
				CurrImage->LoadParams_DDS.Flags &= ~(tImageDDS::LoadFlag_GammaCompression | tImageDDS::LoadFlag_SRGBCompression | tImageDDS::LoadFlag_AutoGamma);
				if (gammaMode == 1) CurrImage->LoadParams_DDS.Flags |= tImageDDS::LoadFlag_GammaCompression;
				if (gammaMode == 2) CurrImage->LoadParams_DDS.Flags |= tImageDDS::LoadFlag_SRGBCompression;
				if (gammaMode == 3) CurrImage->LoadParams_DDS.Flags |= tImageDDS::LoadFlag_AutoGamma;
				reloadChanges = true;
			}
			ImGui::SameLine();
			Gutil::HelpMark
			(
				"Gamma Correction\n"
				"Pixel values may be in linear space. Before being displayed on a screen with non-linear response\n"
				"they should be 'corrected' to gamma or sRGB-space (brightened).\n"
				"\n"
				"None : If you know the source image data is already in either gamma or sRGB-space.\n"
				"Gamma : If you want control over the gamma exponent being used to do the correction. 2.2 is standard.\n"
				"sRGB : If you want to convert to sRGB-space. This more accurately represents a display's response and\n"
				"   is close to a 2.2 gamma but with an extra linear region and a non-unity amplitude.\n"
				"Auto : Let the viewer decide whether to apply sRGB compression based on the detected colour profile.\n",
				false
			);
			if (gammaMode == 1)
			{
				ImGui::SetNextItemWidth(itemWidth);
				if (ImGui::InputFloat("Gamma", &CurrImage->LoadParams_DDS.Gamma, 0.01f, 0.1f, "%.3f"))
					reloadChanges = true;
				ImGui::SameLine();
				Gutil::HelpMark("Gamma to use [0.5, 4.0]. Hold Ctrl to speedup. Open preferences to edit default gamma value.");
				tMath::tiClamp(CurrImage->LoadParams_DDS.Gamma, 0.5f, 4.0f);
			}
			anyUIDisplayed = true;

			if (tIsHDRFormat(CurrImage->Info.SrcPixelFormat) || tIsASTCFormat(CurrImage->Info.SrcPixelFormat))
			{
				bool expEnabled = (CurrImage->LoadParams_DDS.Flags & tImageDDS::LoadFlag_ToneMapExposure);
				ImGui::SetNextItemWidth(itemWidth);
				if (ImGui::InputFloat("Exposure", &CurrImage->LoadParams_DDS.Exposure, 0.001f, 0.05f, "%.4f", expEnabled ? 0 : ImGuiInputTextFlags_ReadOnly))
					reloadChanges = true;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
				ImGui::SameLine();
				if (ImGui::CheckboxFlags("##ExposureEnabled", &CurrImage->LoadParams_DDS.Flags, tImageDDS::LoadFlag_ToneMapExposure))
					reloadChanges = true;
				ImGui::SameLine();
				Gutil::HelpMark("Exposure adjustment [0.0, 4.0]. Hold Ctrl to speedup.");
				tMath::tiClamp(CurrImage->LoadParams_DDS.Exposure, 0.0f, 4.0f);

				anyUIDisplayed = true;
			}

			if (tIsLuminanceFormat(CurrImage->Info.SrcPixelFormat))
			{
				if (ImGui::CheckboxFlags("Spread Luminance", &CurrImage->LoadParams_DDS.Flags, tImageDDS::LoadFlag_SpreadLuminance))
					reloadChanges = true;
				ImGui::SameLine();
				Gutil::HelpMark("Luminance-only dds files are represented in this viewer as having a red channel only,\nIf spread is true, the channel is spread to all RGB channels to create a grey-scale image.");
			}

			bool scrubberDisplayed = false;
			if ((numTextures >= 2) && !altEnabled)
			{
				ImGui::Checkbox("Scrubber", &profile.ShowFrameScrubber);
				anyUIDisplayed = true;
				scrubberDisplayed = true;
			}

			if (anyUIDisplayed)
			{
				if (scrubberDisplayed)
					ImGui::SameLine();

				// The GetWindowContentRegionMax is OK here since width was fixed to a specific size before the Begin call.
				ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - itemWidth);
				if (ImGui::Button("Reset", tVector2(itemWidth, 0.0f)))
				{
					CurrImage->ResetLoadParams();
					CurrImage->FrameNum = 0;
					reloadChanges = true;
				}
			}

			if (reloadChanges)
			{
				CurrImage->Unload();
				CurrImage->Load();
				if (altEnabled)
				{
					CurrImage->EnableAltPicture(true);
					CurrImage->Bind();
				}
			}

			// Some DDS files have no available properties. No textures, no properties.
			// Only one texture and not HDR and no alt images (no mipmaps or cubemap) -> no properties.
			if (!anyUIDisplayed)
				ImGui::Text("No DDS Properties Available");

			ImGui::End();
			return;
		}

		case tSystem::tFileType::PVR:
		{
			bool anyUIDisplayed = false;
			int numTextures = CurrImage->GetNumFrames();
			bool reloadChanges = false;

			anyUIDisplayed |= DoMultiSurface(itemWidth);

			// If we're here show options when have 1 or more frames.
			bool altEnabled = CurrImage->IsAltPictureEnabled();

			// Gamma correction. First read current setting and put it in an int.
			int gammaMode = 0;
			if (CurrImage->LoadParams_PVR.Flags & tImagePVR::LoadFlag_GammaCompression)
				gammaMode = 1;
			if (CurrImage->LoadParams_PVR.Flags & tImagePVR::LoadFlag_SRGBCompression)
				gammaMode = 2;
			if (CurrImage->LoadParams_PVR.Flags & tImagePVR::LoadFlag_AutoGamma)
				gammaMode = 3;
			const char* gammaCorrectItems[] = { "None", "Gamma", "sRGB", "Auto" };
			ImGui::SetNextItemWidth(itemWidth);
			if (ImGui::Combo("Gamma Corr", &gammaMode, gammaCorrectItems, tNumElements(gammaCorrectItems)))
			{
				CurrImage->LoadParams_PVR.Flags &= ~(tImagePVR::LoadFlag_GammaCompression | tImagePVR::LoadFlag_SRGBCompression | tImagePVR::LoadFlag_AutoGamma);
				if (gammaMode == 1) CurrImage->LoadParams_PVR.Flags |= tImagePVR::LoadFlag_GammaCompression;
				if (gammaMode == 2) CurrImage->LoadParams_PVR.Flags |= tImagePVR::LoadFlag_SRGBCompression;
				if (gammaMode == 3) CurrImage->LoadParams_PVR.Flags |= tImagePVR::LoadFlag_AutoGamma;
				reloadChanges = true;
			}
			ImGui::SameLine();
			Gutil::HelpMark
			(
				"Gamma Correction\n"
				"Pixel values may be in linear space. Before being displayed on a screen with non-linear response\n"
				"they should be 'corrected' to gamma or sRGB-space (brightened).\n"
				"\n"
				"None : If you know the source image data is already in either gamma or sRGB-space.\n"
				"Gamma : If you want control over the gamma exponent being used to do the correction. 2.2 is standard.\n"
				"sRGB : If you want to convert to sRGB-space. This more accurately represents a display's response and\n"
				"   is close to a 2.2 gamma but with an extra linear region and a non-unity amplitude.\n"
				"Auto : Let the viewer decide whether to apply sRGB compression based on the detected colour profile.\n",
				false
			);
			if (gammaMode == 1)
			{
				ImGui::SetNextItemWidth(itemWidth);
				if (ImGui::InputFloat("Gamma", &CurrImage->LoadParams_PVR.Gamma, 0.01f, 0.1f, "%.3f"))
					reloadChanges = true;
				ImGui::SameLine();
				Gutil::HelpMark("Gamma to use [0.5, 4.0]. Hold Ctrl to speedup. Open preferences to edit default gamma value.");
				tMath::tiClamp(CurrImage->LoadParams_PVR.Gamma, 0.5f, 4.0f);
			}
			anyUIDisplayed = true;

			if (tIsHDRFormat(CurrImage->Info.SrcPixelFormat) || tIsASTCFormat(CurrImage->Info.SrcPixelFormat))
			{
				bool expEnabled = (CurrImage->LoadParams_PVR.Flags & tImagePVR::LoadFlag_ToneMapExposure);
				ImGui::SetNextItemWidth(itemWidth);
				if (ImGui::InputFloat("Exposure", &CurrImage->LoadParams_PVR.Exposure, 0.001f, 0.05f, "%.4f", expEnabled ? 0 : ImGuiInputTextFlags_ReadOnly))
					reloadChanges = true;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
				ImGui::SameLine();
				if (ImGui::CheckboxFlags("##ExposureEnabled", &CurrImage->LoadParams_PVR.Flags, tImagePVR::LoadFlag_ToneMapExposure))
					reloadChanges = true;
				ImGui::SameLine();
				Gutil::HelpMark("Exposure adjustment [0.0, 4.0]. Hold Ctrl to speedup.");
				tMath::tiClamp(CurrImage->LoadParams_PVR.Exposure, 0.0f, 4.0f);

				anyUIDisplayed = true;
			}

			if ((CurrImage->Info.SrcPixelFormat == tPixelFormat::R8G8B8M8) || (CurrImage->Info.SrcPixelFormat == tPixelFormat::R8G8B8D8))
			{
				ImGui::SetNextItemWidth(itemWidth);
				if (ImGui::InputFloat("MaxRange", &CurrImage->LoadParams_PVR.MaxRange, 0.01f, 1.0f, "%.3f"))
					reloadChanges = true;
				ImGui::SameLine();
				Gutil::HelpMark("Max range to use [0.01, 128.0] for decoding RGBM and RGBD images. Hold Ctrl to speedup.");
				tMath::tiClamp(CurrImage->LoadParams_PVR.MaxRange, 0.01f, 128.0f);
			}

			if (tIsLuminanceFormat(CurrImage->Info.SrcPixelFormat))
			{
				if (ImGui::CheckboxFlags("Spread Luminance", &CurrImage->LoadParams_PVR.Flags, tImagePVR::LoadFlag_SpreadLuminance))
					reloadChanges = true;
				ImGui::SameLine();
				Gutil::HelpMark("Luminance-only pvr files are represented in this viewer as having a red channel only,\nIf spread is true, the channel is spread to all RGB channels to create a grey-scale image.");
			}

			bool scrubberDisplayed = false;
			if ((numTextures >= 2) && !altEnabled)
			{
				ImGui::Checkbox("Scrubber", &profile.ShowFrameScrubber);
				anyUIDisplayed = true;
				scrubberDisplayed = true;
			}

			if (anyUIDisplayed)
			{
				if (scrubberDisplayed)
					ImGui::SameLine();

				// The GetWindowContentRegionMax is OK here since width was fixed to a specific size before the Begin call.
				ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - itemWidth);
				if (ImGui::Button("Reset", tVector2(itemWidth, 0.0f)))
				{
					CurrImage->ResetLoadParams();
					CurrImage->FrameNum = 0;
					reloadChanges = true;
				}
			}

			if (reloadChanges)
			{
				CurrImage->Unload();
				CurrImage->Load();
				if (altEnabled)
				{
					CurrImage->EnableAltPicture(true);
					CurrImage->Bind();
				}
			}

			// Some PVR files have no available properties. No textures, no properties.
			// Only one texture and not HDR and no alt images (no mipmaps or cubemap) -> no properties.
			if (!anyUIDisplayed)
				ImGui::Text("No PVR Properties Available");

			ImGui::End();
			return;
		}

		case tSystem::tFileType::KTX:
		case tSystem::tFileType::KTX2:
		{
			bool anyUIDisplayed = false;
			int numTextures = CurrImage->GetNumFrames();
			bool reloadChanges = false;

			anyUIDisplayed |= DoMultiSurface(itemWidth);

			if (tIsETCFormat(CurrImage->Info.SrcPixelFormat))
			{
				if (ImGui::CheckboxFlags("SwizzleBGRToRGB", &CurrImage->LoadParams_KTX.Flags, tImageKTX::LoadFlag_SwizzleBGR2RGB))
					reloadChanges = true;
				anyUIDisplayed = true;
			}
		
			// If we're here show options when have 1 or more frames.
			bool altEnabled = CurrImage->IsAltPictureEnabled();

			// Gamma correction. First read current setting and put it in an int.
			int gammaMode = 0;
			if (CurrImage->LoadParams_KTX.Flags & tImageKTX::LoadFlag_GammaCompression)
				gammaMode = 1;
			if (CurrImage->LoadParams_KTX.Flags & tImageKTX::LoadFlag_SRGBCompression)
				gammaMode = 2;
			if (CurrImage->LoadParams_KTX.Flags & tImageKTX::LoadFlag_AutoGamma)
				gammaMode = 3;
			const char* gammaCorrectItems[] = { "None", "Gamma", "sRGB", "Auto" };
			ImGui::SetNextItemWidth(itemWidth);
			if (ImGui::Combo("Gamma Corr", &gammaMode, gammaCorrectItems, tNumElements(gammaCorrectItems)))
			{
				CurrImage->LoadParams_KTX.Flags &= ~(tImageKTX::LoadFlag_GammaCompression | tImageKTX::LoadFlag_SRGBCompression | tImageKTX::LoadFlag_AutoGamma);
				if (gammaMode == 1) CurrImage->LoadParams_KTX.Flags |= tImageKTX::LoadFlag_GammaCompression;
				if (gammaMode == 2) CurrImage->LoadParams_KTX.Flags |= tImageKTX::LoadFlag_SRGBCompression;
				if (gammaMode == 3) CurrImage->LoadParams_KTX.Flags |= tImageKTX::LoadFlag_AutoGamma;
				reloadChanges = true;
			}
			ImGui::SameLine();
			Gutil::HelpMark
			(
				"Gamma Correction\n"
				"Pixel values may be in linear space. Before being displayed on a screen with non-linear response\n"
				"they should be 'corrected' to gamma or sRGB-space (brightened).\n"
				"\n"
				"None : If you know the source image data is already in either gamma or sRGB-space.\n"
				"Gamma : If you want control over the gamma exponent being used to do the correction. 2.2 is standard.\n"
				"sRGB : If you want to convert to sRGB-space. This more accurately represents a display's response and\n"
				"   is close to a 2.2 gamma but with an extra linear region and a non-unity amplitude.\n"
				"Auto : Let the viewer decide whether to apply sRGB compression based on the detected colour profile.\n",
				false
			);
			if (gammaMode == 1)
			{
				ImGui::SetNextItemWidth(itemWidth);
				if (ImGui::InputFloat("Gamma", &CurrImage->LoadParams_KTX.Gamma, 0.01f, 0.1f, "%.3f"))
					reloadChanges = true;
				ImGui::SameLine();
				Gutil::HelpMark("Gamma to use [0.5, 4.0]. Hold Ctrl to speedup. Open preferences to edit default gamma value.");
				tMath::tiClamp(CurrImage->LoadParams_KTX.Gamma, 0.5f, 4.0f);
			}
			anyUIDisplayed = true;

			// We don't allow exposure control on ETC and EAC images.
			if
			(
				(!tIsETCFormat(CurrImage->Info.SrcPixelFormat) && !tIsEACFormat(CurrImage->Info.SrcPixelFormat)) &&
				(tIsHDRFormat(CurrImage->Info.SrcPixelFormat) || tIsProfileLinearInRGB(CurrImage->Info.SrcColourProfile))
			)
			{
				bool expEnabled = (CurrImage->LoadParams_KTX.Flags & tImageKTX::LoadFlag_ToneMapExposure);
				ImGui::SetNextItemWidth(itemWidth);
				if (ImGui::InputFloat("Exposure", &CurrImage->LoadParams_KTX.Exposure, 0.001f, 0.05f, "%.4f", expEnabled ? 0 : ImGuiInputTextFlags_ReadOnly))
					reloadChanges = true;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
				ImGui::SameLine();
				if (ImGui::CheckboxFlags("##ExposureEnabled", &CurrImage->LoadParams_KTX.Flags, tImageKTX::LoadFlag_ToneMapExposure))
					reloadChanges = true;
				ImGui::SameLine();
				Gutil::HelpMark("Exposure adjustment [0.0, 4.0]. Hold Ctrl to speedup.");
				tMath::tiClamp(CurrImage->LoadParams_KTX.Exposure, 0.0f, 4.0f);

				anyUIDisplayed = true;
			}

			if (tIsLuminanceFormat(CurrImage->Info.SrcPixelFormat))
			{
				if (ImGui::CheckboxFlags("Spread Luminance", &CurrImage->LoadParams_KTX.Flags, tImageKTX::LoadFlag_SpreadLuminance))
					reloadChanges = true;
				ImGui::SameLine();
				Gutil::HelpMark("Luminance-only ktx/ktx2 files are represented in this viewer as having a red channel only,\nIf spread is true, the channel is spread to all RGB channels to create a grey-scale image.");
			}

			bool scrubberDisplayed = false;
			if ((numTextures >= 2) && !altEnabled)
			{
				ImGui::Checkbox("Scrubber", &profile.ShowFrameScrubber);
				anyUIDisplayed = true;
				scrubberDisplayed = true;
			}

			if (anyUIDisplayed)
			{
				if (scrubberDisplayed)
					ImGui::SameLine();

				// The GetWindowContentRegionMax is OK here since width was fixed to a specific size before the Begin call.
				ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - itemWidth);
				if (ImGui::Button("Reset", tVector2(itemWidth, 0.0f)))
				{
					CurrImage->ResetLoadParams();
					CurrImage->FrameNum = 0;
					reloadChanges = true;
				}
			}

			if (reloadChanges)
			{
				CurrImage->Unload();
				CurrImage->Load();
				if (altEnabled)
				{
					CurrImage->EnableAltPicture(true);
					CurrImage->Bind();
				}
			}

			// Some KTX/KTX2 files have no available properties. No textures, no properties.
			// If only one texture and not HDR and no alt images (no mipmaps or cubemap) -> no properties.
			if (!anyUIDisplayed)
				ImGui::Text("No KTX/KTX2 Properties Available");

			ImGui::End();
			return;
		}

		case tSystem::tFileType::ASTC:
		{
			bool reloadChanges = false;

			int colourProfile = int(CurrImage->LoadParams_ASTC.Profile);			
			ImGui::SetNextItemWidth(itemWidth);
			if (ImGui::Combo("Colour Profile", &colourProfile, tColourProfileShortNames, tNumElements(tColourProfileShortNames)-1))
			{
				CurrImage->LoadParams_ASTC.Profile = tColourProfile(colourProfile);
				reloadChanges = true;
			}
			ImGui::SameLine();
			Gutil::HelpMark
			(
				"Colour Profile\n"
				"ASTC files do not contain information about the colour profile so we supply it here.\n"
				"Most LDR (low-dynamic-range) images have their colours authored in sRGB space since that is what your\n"
				"monitor displays. If there is an alpha, it is usually in linear-space and clamped to the range [0.0, 1.0].\n"
				"The ASTC decoder needs to know what kind of pixel data it is dealing with. HDR (high-dynamic-range) just\n"
				"means the pixel data can be outside the [0.0, 1.0] range. LDR means it is within it. The space is either\n"
				"'Linear' or 'sRGB'. Generally HDR images are in linear space. The Colour Profile determines how both the\n"
				"space and the range should be interpreted for each channel of an image.\n"
				"\n"
				"sRGB : LDRsRGB_LDRlA. LDR RGB-components in sRGB space. LDR alpha in linear space. Most LDR images are this.\n"
				"gRGB : LDRgRGB_LDRlA. LDR RGB-components in gRGB space. LDR alpha in linear space.\n"
				"lRGB : LDRlRGBA. LDR RGBA-components all in linear space. Normal maps often use this.\n"
				"HDRa : HDR RGB-components in linear space. LDR alpha in linear space. Most HDR images are this.\n"
				"HDRA : HDR RGBA-components all in linear space.",
				false
			);

			// Gamma correction. First read current setting and put it in an int.
			int gammaMode = 0;
			if (CurrImage->LoadParams_ASTC.Flags & tImageASTC::LoadFlag_GammaCompression)
				gammaMode = 1;
			if (CurrImage->LoadParams_ASTC.Flags & tImageASTC::LoadFlag_SRGBCompression)
				gammaMode = 2;
			if (CurrImage->LoadParams_ASTC.Flags & tImageASTC::LoadFlag_AutoGamma)
				gammaMode = 3;
			const char* gammaCorrectItems[] = { "None", "Gamma", "sRGB", "Auto" };
			ImGui::SetNextItemWidth(itemWidth);
			if (ImGui::Combo("Gamma Corr", &gammaMode, gammaCorrectItems, tNumElements(gammaCorrectItems)))
			{
				CurrImage->LoadParams_ASTC.Flags &= ~(tImageASTC::LoadFlag_GammaCompression | tImageASTC::LoadFlag_SRGBCompression | tImageASTC::LoadFlag_AutoGamma);
				if (gammaMode == 1) CurrImage->LoadParams_ASTC.Flags |= tImageASTC::LoadFlag_GammaCompression;
				if (gammaMode == 2) CurrImage->LoadParams_ASTC.Flags |= tImageASTC::LoadFlag_SRGBCompression;
				if (gammaMode == 3) CurrImage->LoadParams_ASTC.Flags |= tImageASTC::LoadFlag_AutoGamma;
				reloadChanges = true;
			}
			ImGui::SameLine();
			Gutil::HelpMark
			(
				"Gamma Correction\n"
				"Pixel values may be in linear space. Before being displayed on a screen with non-linear response\n"
				"they should be 'corrected' to gamma or sRGB-space (brightened).\n"
				"\n"
				"None : If you know the source image data is already in either gamma or sRGB-space.\n"
				"Gamma : If you want control over the gamma exponent being used to do the correction. 2.2 is standard.\n"
				"sRGB : If you want to convert to sRGB-space. This more accurately represents a display's response and\n"
				"   is close to a 2.2 gamma but with an extra linear region and a non-unity amplitude.\n"
				"Auto : Let the viewer decide whether to apply sRGB compression based on the detected colour profile.\n",
				false
			);
			if (gammaMode == 1)
			{
				ImGui::SetNextItemWidth(itemWidth);
				if (ImGui::InputFloat("Gamma", &CurrImage->LoadParams_ASTC.Gamma, 0.01f, 0.1f, "%.3f"))
					reloadChanges = true;
				ImGui::SameLine();
				Gutil::HelpMark("Gamma to use [0.5, 4.0]. Hold Ctrl to speedup. Open preferences to edit default gamma value.");
				tMath::tiClamp(CurrImage->LoadParams_ASTC.Gamma, 0.5f, 4.0f);
			}

			// @todo Add detection of HDR blocks to tImageASTC.
			// if (tIsHDRFormat(CurrImage->Info.SrcPixelFormat) || (CurrImage->Info.SrcColourSpace == tColourSpace::Linear))
			if (1)
			{
				bool expEnabled = (CurrImage->LoadParams_ASTC.Flags & tImageASTC::LoadFlag_ToneMapExposure);
				ImGui::SetNextItemWidth(itemWidth);
				if (ImGui::InputFloat("Exposure", &CurrImage->LoadParams_ASTC.Exposure, 0.001f, 0.05f, "%.4f", expEnabled ? 0 : ImGuiInputTextFlags_ReadOnly))
					reloadChanges = true;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
				ImGui::SameLine();
				if (ImGui::CheckboxFlags("##ExposureEnabled", &CurrImage->LoadParams_ASTC.Flags, tImageASTC::LoadFlag_ToneMapExposure))
					reloadChanges = true;
				ImGui::SameLine();
				Gutil::HelpMark("Exposure adjustment [0.0, 4.0]. Hold Ctrl to speedup.");
				tMath::tiClamp(CurrImage->LoadParams_ASTC.Exposure, 0.0f, 4.0f);
			}

			// The GetWindowContentRegionMax is OK here since width was fixed to a specific size before the Begin call.
			ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - itemWidth);
			if (ImGui::Button("Reset", tVector2(itemWidth, 0.0f)))
			{
				CurrImage->ResetLoadParams();
				CurrImage->FrameNum = 0;
				reloadChanges = true;
			}

			if (reloadChanges)
			{
				CurrImage->Unload();
				CurrImage->Load();
			}

			ImGui::End();
			return;
		}

		case tSystem::tFileType::PKM:
		{
			bool reloadChanges = false;
			tString texTypeName = "Texture";

			// Gamma correction. First read current setting and put it in an int.
			int gammaMode = 0;
			if (CurrImage->LoadParams_PKM.Flags & tImagePKM::LoadFlag_GammaCompression)
				gammaMode = 1;
			if (CurrImage->LoadParams_PKM.Flags & tImagePKM::LoadFlag_SRGBCompression)
				gammaMode = 2;
			if (CurrImage->LoadParams_PKM.Flags & tImagePKM::LoadFlag_AutoGamma)
				gammaMode = 3;
			const char* gammaCorrectItems[] = { "None", "Gamma", "sRGB", "Auto" };
			ImGui::SetNextItemWidth(itemWidth);
			if (ImGui::Combo("Gamma Corr", &gammaMode, gammaCorrectItems, tNumElements(gammaCorrectItems)))
			{
				CurrImage->LoadParams_PKM.Flags &= ~(tImagePKM::LoadFlag_GammaCompression | tImagePKM::LoadFlag_SRGBCompression | tImagePKM::LoadFlag_AutoGamma);
				if (gammaMode == 1) CurrImage->LoadParams_PKM.Flags |= tImagePKM::LoadFlag_GammaCompression;
				if (gammaMode == 2) CurrImage->LoadParams_PKM.Flags |= tImagePKM::LoadFlag_SRGBCompression;
				if (gammaMode == 3) CurrImage->LoadParams_PKM.Flags |= tImagePKM::LoadFlag_AutoGamma;
				reloadChanges = true;
			}
			ImGui::SameLine();
			Gutil::HelpMark
			(
				"Gamma Correction\n"
				"Pixel values may be in linear space. Before being displayed on a screen with non-linear response\n"
				"they should be 'corrected' to gamma or sRGB-space (brightened).\n"
				"\n"
				"None : If you know the source image data is already in either gamma or sRGB-space.\n"
				"Gamma : If you want control over the gamma exponent being used to do the correction. 2.2 is standard.\n"
				"sRGB : If you want to convert to sRGB-space. This more accurately represents a display's response and\n"
				"   is close to a 2.2 gamma but with an extra linear region and a non-unity amplitude.\n"
				"Auto : Let the viewer decide whether to apply sRGB compression based on the detected colour profile.\n",
				false
			);
			if (gammaMode == 1)
			{
				ImGui::SetNextItemWidth(itemWidth);
				if (ImGui::InputFloat("Gamma", &CurrImage->LoadParams_PKM.Gamma, 0.01f, 0.1f, "%.3f"))
					reloadChanges = true;
				ImGui::SameLine();
				Gutil::HelpMark("Gamma to use [0.5, 4.0]. Hold Ctrl to speedup. Open preferences to edit default gamma value.");
				tMath::tiClamp(CurrImage->LoadParams_PKM.Gamma, 0.5f, 4.0f);
			}

			if (tIsLuminanceFormat(CurrImage->Info.SrcPixelFormat))
			{
				if (ImGui::CheckboxFlags("Spread Luminance", &CurrImage->LoadParams_PKM.Flags, tImagePKM::LoadFlag_SpreadLuminance))
					reloadChanges = true;
				ImGui::SameLine();
				Gutil::HelpMark("Luminance-only pkm files are represented in this viewer as having a red channel only,\nIf spread is true, the channel is spread to all RGB channels to create a grey-scale image.");
			}

			// The GetWindowContentRegionMax is OK here since width was fixed to a specific size before the Begin call.
			ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - itemWidth);
			if (ImGui::Button("Reset", tVector2(itemWidth, 0.0f)))
			{
				CurrImage->ResetLoadParams();
				reloadChanges = true;
			}

			if (reloadChanges)
			{
				CurrImage->Unload();
				CurrImage->Load();
			}

			ImGui::End();
			return;
		}

		case tSystem::tFileType::HDR:
		{
			ImGui::Text("Radiance HDR");
			bool reloadChanges = false;

			ImGui::SetNextItemWidth(itemWidth);
			if (ImGui::InputFloat("Gamma", &CurrImage->LoadParams_HDR.Gamma, 0.01f, 0.1f, "%.3f"))
				reloadChanges = true;
			ImGui::SameLine();
			Gutil::HelpMark("Gamma to use [0.6, 3.0]. Hold Ctrl to speedup. Open preferences to edit default gamma value.");
			tMath::tiClamp(CurrImage->LoadParams_HDR.Gamma, 0.6f, 3.0f);

			ImGui::SetNextItemWidth(itemWidth);
			if (ImGui::InputInt("Exposure", &CurrImage->LoadParams_HDR.Exposure))
				reloadChanges = true;
			ImGui::SameLine();
			Gutil::HelpMark("Exposure adjustment [-10, 10]. Hold Ctrl to speedup.");
			tMath::tiClamp(CurrImage->LoadParams_HDR.Exposure, -10, 10);

			// The GetWindowContentRegionMax is OK here since width was fixed to a specific size before the Begin call.
			ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - itemWidth);
			if (ImGui::Button("Reset", tVector2(itemWidth, 0.0f)))
			{
				CurrImage->ResetLoadParams();
				reloadChanges = true;
			}

			if (reloadChanges)
			{
				CurrImage->Unload();
				CurrImage->Load();
			}

			fileTypeSectionDisplayed = true;
			break;
		}

		case tSystem::tFileType::EXR:
		{
			ImGui::Text("OpenEXR");
			bool reloadChanges = false;

			ImGui::SetNextItemWidth(itemWidth);
			if (ImGui::InputFloat("Gamma", &CurrImage->LoadParams_EXR.Gamma, 0.01f, 0.1f, "%.3f"))
				reloadChanges = true;
			ImGui::SameLine();
			Gutil::HelpMark("Gamma to use [0.6, 3.0]. Hold Ctrl to speedup. Open preferences to edit default gamma value.");
			tMath::tiClamp(CurrImage->LoadParams_EXR.Gamma, 0.6f, 3.0f);

			ImGui::SetNextItemWidth(itemWidth);
			if (ImGui::InputFloat("Exposure", &CurrImage->LoadParams_EXR.Exposure, 0.01f, 0.1f, "%.3f"))
				reloadChanges = true;
			ImGui::SameLine();
			Gutil::HelpMark("Exposure adjustment [-10.0, 10.0]. Hold Ctrl to speedup.");
			tMath::tiClamp(CurrImage->LoadParams_EXR.Exposure, -10.0f, 10.0f);

			ImGui::SetNextItemWidth(itemWidth);
			if (ImGui::InputFloat("Defog", &CurrImage->LoadParams_EXR.Defog, 0.001f, 0.01f, "%.3f"))
				reloadChanges = true;
			ImGui::SameLine();
			Gutil::HelpMark("Remove fog strength [0.0, 0.1]. Hold Ctrl to speedup. Try to keep under 0.01");
			tMath::tiClamp(CurrImage->LoadParams_EXR.Defog, 0.0f, 0.1f);

			ImGui::SetNextItemWidth(itemWidth);
			if (ImGui::InputFloat("Knee Low", &CurrImage->LoadParams_EXR.KneeLow, 0.01f, 0.1f, "%.3f"))
				reloadChanges = true;
			ImGui::SameLine();
			Gutil::HelpMark("Lower bound knee taper [-3.0, 3.0]. Hold Ctrl to speedup.");
			tMath::tiClamp(CurrImage->LoadParams_EXR.KneeLow, -3.0f, 3.0f);

			ImGui::SetNextItemWidth(itemWidth);
			if (ImGui::InputFloat("Knee High", &CurrImage->LoadParams_EXR.KneeHigh, 0.01f, 0.1f, "%.3f"))
				reloadChanges = true;
			ImGui::SameLine();
			Gutil::HelpMark("Upper bound knee taper [3.5, 7.5]. Hold Ctrl to speedup.");
			tMath::tiClamp(CurrImage->LoadParams_EXR.KneeHigh, 3.5f, 7.5f);

			// The GetWindowContentRegionMax is OK here since width was fixed to a specific size before the Begin call.
			ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - itemWidth);
			if (ImGui::Button("Reset", tVector2(itemWidth, 0.0f)))
			{
				CurrImage->ResetLoadParams();
				reloadChanges = true;
			}

			if (reloadChanges)
			{
				CurrImage->Unload();
				CurrImage->Load();
			}

			fileTypeSectionDisplayed = true;
			break;
		}

		case tSystem::tFileType::TGA:
		{
			if ((CurrImage->Info.SrcPixelFormat == tPixelFormat::R8G8B8A8) || (CurrImage->Info.SrcPixelFormat == tPixelFormat::G3B5A1R5G2))
			{
				ImGui::Text("Truevision TGA");
				bool reloadChanges = false;

				ImGui::SetNextItemWidth(itemWidth);
				if (ImGui::CheckboxFlags("Alpha Is Opacity", &CurrImage->LoadParams_TGA.Flags, tImageTGA::LoadFlag_AlphaOpacity))
					reloadChanges = true;
				ImGui::SameLine();
				Gutil::HelpMark
				(
					"The most common way to interpret the alpha channel is\n"
					"as opacity (0.0 is fully transparent, 1.0 is fully opaque).\n"
					"There are some TGAs (especially 16-bit 5551) in the\n"
					"wild that are saved with a 0 in the alpha channel and\n"
					"are expected to be visible.\n"
					"\n"
					"Checked   : Normal (alpha is opacity)\n"
					"  0 = transparent. 1 = opaque.\n"
					"\n"
					"Unchecked : Reversed (alpha is transparency)\n"
					"  0 = opaque. 1 = transparent."
				);

				// The GetWindowContentRegionMax is OK here since width was fixed to a specific size before the Begin call.
				ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - itemWidth);
				if (ImGui::Button("Reset", tVector2(itemWidth, 0.0f)))
				{
					CurrImage->ResetLoadParams();
					reloadChanges = true;
				}

				if (reloadChanges)
				{
					CurrImage->Unload();
					CurrImage->Load();
				}
				fileTypeSectionDisplayed = true;
			}
			break;
		}

		case tSystem::tFileType::WEBP:
		{
			if (CurrImage->Info.Opacity != Image::Image::ImgInfo::OpacityEnum::True)
			{
				ImGui::Checkbox("Override Background", &CurrImage->OverrideBackgroundColour);
				ImGui::SameLine();
				Gutil::HelpMark
				(
					"WebP files store a background canvas colour. This canvas colour is present\n"
					"in animated WebP files and defaults to white for single image WebP files.\n"
					"If the override checkbox is set the current viewer background settings are\n"
					"ignored and the WebP background colour is used instead. This only affects\n"
					"the current image being displayed."
				);

				fileTypeSectionDisplayed = true;
			}
			break;
		}
	}

	int numFrames = CurrImage->GetNumFrames();
	if ((numFrames <= 1) && !fileTypeSectionDisplayed)
		ImGui::Text("No Editable Image Properties Available");

	if (numFrames > 1)
	{
		if (fileTypeSectionDisplayed)
			ImGui::Separator();

		int oneBasedFrameNum = CurrImage->FrameNum + 1;
		tString frameStr;
		tsPrintf(frameStr, "Frame (%d)##Frame", CurrImage->GetNumFrames());
		ImGui::SetNextItemWidth(itemWidth);
		if (ImGui::InputInt(frameStr.Chr(), &oneBasedFrameNum))
		{
			CurrImage->FrameNum = oneBasedFrameNum - 1;
			tMath::tiClamp(CurrImage->FrameNum, 0, CurrImage->GetNumFrames()-1);
		}
		ImGui::SameLine(); Gutil::HelpMark("Which image in a multiframe file to display.");

		float durButtonSpacing = Gutil::GetUIParamExtent(4.0f, 10.0f);
		const ImGuiStyle& style = ImGui::GetStyle();

		if (CurrImage->FrameDurationPreviewEnabled)
		{
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputFloat("Period", &CurrImage->FrameDurationPreview, 0.01f, 0.1f, "%.4f");

			tMath::tiClamp(CurrImage->FrameDurationPreview, 0.0f, 60.0f);
			ImGui::SameLine();
			if (ImGui::Button("Set All"))
			{
				CurrImage->SetFrameDuration(CurrImage->FrameDurationPreview, true);
				Gutil::SetWindowTitle();
				CurrImage->FrameDurationPreviewEnabled = false;
			}
			ImGui::SameLine(); Gutil::HelpMark("Sets every frame period to the preview period in seconds.");

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, tVector2(durButtonSpacing, style.ItemSpacing.y));
			if (ImGui::Button("1.0s"))	CurrImage->FrameDurationPreview = 1.0f;			ImGui::SameLine();
			if (ImGui::Button("0.5s"))	CurrImage->FrameDurationPreview = 0.5f;			ImGui::SameLine();
			if (ImGui::Button("0.1s"))	CurrImage->FrameDurationPreview = 0.1f;			ImGui::SameLine();
			if (ImGui::Button("30Hz"))	CurrImage->FrameDurationPreview = 1.0f/30.0f;	ImGui::SameLine();
			if (ImGui::Button("60Hz"))	CurrImage->FrameDurationPreview = 1.0f/60.0f;	ImGui::SameLine();
			Gutil::HelpMark("Predefined frame period buttons.");
			ImGui::PopStyleVar();
		}
		else
		{
			tPicture* currFramePic = CurrImage->GetCurrentPic();
			float duration = currFramePic->Duration;
			char frameDurText[64];
			ImGui::SetNextItemWidth(itemWidth);
			if (ImGui::InputFloat("Period", &duration, 0.01f, 0.1f, "%.4f", ImGuiInputTextFlags_EnterReturnsTrue))
			{
				tMath::tiClamp(duration, 0.0f, 60.0f);
				CurrImage->SetFrameDuration(duration);
				Gutil::SetWindowTitle();
			}
			ImGui::SameLine(); Gutil::HelpMark("This frame's period in seconds.");

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, tVector2(durButtonSpacing, style.ItemSpacing.y));
			if (ImGui::Button("1.0s"))	{ CurrImage->SetFrameDuration(1.0f); Gutil::SetWindowTitle(); }			ImGui::SameLine();
			if (ImGui::Button("0.5s"))	{ CurrImage->SetFrameDuration(0.5f); Gutil::SetWindowTitle(); }			ImGui::SameLine();
			if (ImGui::Button("0.1s"))	{ CurrImage->SetFrameDuration(0.1f); Gutil::SetWindowTitle(); }			ImGui::SameLine();
			if (ImGui::Button("30Hz"))	{ CurrImage->SetFrameDuration(1.0f/30.0f); Gutil::SetWindowTitle(); }	ImGui::SameLine();
			if (ImGui::Button("60Hz"))	{ CurrImage->SetFrameDuration(1.0f/60.0f); Gutil::SetWindowTitle(); }	ImGui::SameLine();
			Gutil::HelpMark("Predefined frame period buttons.");
			ImGui::PopStyleVar();
		}
		ImGui::Checkbox("Preview Period", &CurrImage->FrameDurationPreviewEnabled);
		ImGui::SameLine(); Gutil::HelpMark("If enabled this number of seconds is used for all frame periods while playing.");
		ImGui::Checkbox("Scrubber", &profile.ShowFrameScrubber);

		Gutil::Separator();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);

		float itemHSpacing = Gutil::GetUIParamExtent(8.0f, 32.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, tVector2(itemHSpacing, style.ItemSpacing.y));

		uint64 loopImageID = CurrImage->FramePlayLooping ? Image_PlayOnce.Bind() : Image_PlayLoop.Bind();
		if (ImGui::ImageButton(ImTextureID(loopImageID), imgButtonSize, tVector2(0.0f, 1.0f), tVector2(1.0f, 0.0f), 2, ColourBG, ColourEnabledTint))
			CurrImage->FramePlayLooping = !CurrImage->FramePlayLooping;
		ImGui::SameLine();

		bool prevEnabled = !CurrImage->FramePlaying && (CurrImage->FrameNum > 0);
		ImGui::PushID("PropSkipBegin");
		if (ImGui::ImageButton
		(
			ImTextureID(Image_SkipEnd_SkipBegin.Bind()), imgButtonSize, tVector2(1.0f, 1.0f), tVector2(0.0f, 0.0f), 2,
			ColourBG, prevEnabled ? ColourEnabledTint : ColourDisabledTint) && prevEnabled
		)	CurrImage->FrameNum = 0;
		ImGui::PopID();
		ImGui::SameLine();

		ImGui::PushID("PropPrev");
		if (ImGui::ImageButton
		(
			ImTextureID(Image_Next_Prev.Bind()), imgButtonSize, tVector2(1.0f, 1.0f), tVector2(0.0f, 0.0f), 2,
			ColourBG, prevEnabled ? ColourEnabledTint : ColourDisabledTint) && prevEnabled
		)	CurrImage->FrameNum = tClampMin(CurrImage->FrameNum-1, 0);
		ImGui::PopID();
		ImGui::SameLine();

		bool playRevEnabled = !(CurrImage->FramePlaying && !CurrImage->FramePlayRev);
		uint64 playRevImageID = !CurrImage->FramePlaying ? Image_Play_PlayRev.Bind() : Image_Stop.Bind();
		ImGui::PushID("PropPlayRev");
		if (ImGui::ImageButton
		(
			ImTextureID(playRevImageID), imgButtonSize, tVector2(1.0f, 1.0f), tVector2(0.0f, 0.0f), 2,
			ColourBG, playRevEnabled ? ColourEnabledTint : ColourDisabledTint) && playRevEnabled
		)
		{
			CurrImage->FramePlayRev = true;
			if (CurrImage->FramePlaying) CurrImage->Stop(); else CurrImage->Play();
		}
		ImGui::PopID();
		ImGui::SameLine();

		bool playFwdEnabled = !(CurrImage->FramePlaying && CurrImage->FramePlayRev);
		uint64 playImageID = !CurrImage->FramePlaying ? Image_Play_PlayRev.Bind() : Image_Stop.Bind();
		ImGui::PushID("PropPlayFwd");
		if (ImGui::ImageButton
		(
			ImTextureID(playImageID), imgButtonSize, tVector2(0.0f, 1.0f), tVector2(1.0f, 0.0f), 2,
			ColourBG, playFwdEnabled ? ColourEnabledTint : ColourDisabledTint) && playFwdEnabled
		)
		{
			CurrImage->FramePlayRev = false;
			if (CurrImage->FramePlaying) CurrImage->Stop(); else CurrImage->Play();
		}
		ImGui::PopID();
		ImGui::SameLine();

		bool nextEnabled = !CurrImage->FramePlaying && (CurrImage->FrameNum < (numFrames-1));
		ImGui::PushID("PropNext");
		if (ImGui::ImageButton
		(
			ImTextureID(Image_Next_Prev.Bind()), imgButtonSize, tVector2(0.0f, 1.0f), tVector2(1.0f, 0.0f), 2,
			ColourBG, nextEnabled ? ColourEnabledTint : ColourDisabledTint) && nextEnabled
		)	CurrImage->FrameNum = tClampMax(CurrImage->FrameNum+1, numFrames-1);
		ImGui::PopID();
		ImGui::SameLine();

		ImGui::PushID("PropSkipEnd");
		if (ImGui::ImageButton
		(
			ImTextureID(Image_SkipEnd_SkipBegin.Bind()), imgButtonSize, tVector2(0.0f, 1.0f), tVector2(1.0f, 0.0f), 2,
			ColourBG, nextEnabled ? ColourEnabledTint : ColourDisabledTint) && nextEnabled
		)	CurrImage->FrameNum = numFrames-1;
		ImGui::PopID();
		ImGui::SameLine();

		ImGui::PopStyleVar();
	}

	ImGui::End();
}
