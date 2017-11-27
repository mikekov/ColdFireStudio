#pragma once


class JpegPicture
{
public:
	JpegPicture() : picture_(nullptr)
	{}

	~JpegPicture()
	{ free(); }

	bool LoadPicture(HINSTANCE res_handle, int jpeg_res_id);

	CBitmap& GetBitmap() { return bitmap_; }

private:
	void free();

	CBitmap bitmap_;
	IPicture* picture_;
};
