/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "Renderers/DXCommon/GSRendererDX.h"
#include "Renderers/HW/GSVertexHW.h"
#include "GSTextureCache9.h"

class GSRendererDX9 : public GSRendererDX
{
protected:
	struct
	{
		Direct3DDepthStencilState9 dss;
		Direct3DBlendState9 bs;
	} m_fba;

	void EmulateTextureShuffleAndFbmask();
	void SetupIA(const float& sx, const float& sy);
	void UpdateFBA(GSTexture* rt);

public:
	GSRendererDX9();
	virtual ~GSRendererDX9() {}

	bool CreateDevice(GSDevice* dev);
};
