/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "load_jpeg.h"


namespace {
IPicture* LoadPicture(const void* data, size_t len)
{
	HGLOBAL global= ::GlobalAlloc(GMEM_MOVEABLE, len);
	if (global == nullptr)
		return nullptr;
	LPVOID pvData= ::GlobalLock(global);
	if (pvData == nullptr)
		return nullptr;
	memcpy(pvData, data, len);
	::GlobalUnlock(global);
	LPSTREAM stream= 0;
	::CreateStreamOnHGlobal(global, true, &stream);
	IPicture* pic= nullptr;
	::OleLoadPicture(stream, 0, false, IID_IPicture, reinterpret_cast<void**>(&pic));
	// release stream and global memory too (due to second param to CreateStreamOnHGlobal being true)
	stream->Release();
	return pic;
}
}


void JpegPicture::free()
{
	if (picture_)
	{
		bitmap_.Detach();
		picture_->Release();
	}
	picture_ = nullptr;
}


bool JpegPicture::LoadPicture(HINSTANCE res_handle, int jpeg_res_id)
{
	free();

	if (res_handle == nullptr)
		res_handle = AfxGetResourceHandle();
	if (HRSRC res= FindResource(res_handle, MAKEINTRESOURCE(jpeg_res_id), L"JPEG"))
		if (HGLOBAL mem= LoadResource(res_handle, res))
		{
			void* data= LockResource(mem);
			size_t size= SizeofResource(res_handle, res);

			if (picture_ = ::LoadPicture(data, size))
			{
				HBITMAP bitmap;
				if (picture_->get_Handle(reinterpret_cast<OLE_HANDLE*>(&bitmap)) == S_OK)
					bitmap_.Attach(bitmap);
				// we need to keep picture_ around; it owns the bitmap
			}
		}

	return bitmap_.m_hObject != nullptr;
}
