#pragma once
#ifndef HostBitmap_H
#define HostBitmap_H

class Tr2ImageHandler;

namespace ImageIO
{

class HostBitmap: public Tr2BitmapDimensions
{
public:
    HostBitmap();
	virtual ~HostBitmap();

	bool IsValid() const;
	virtual void Destroy();

	bool Create( unsigned width, unsigned height, unsigned mipCount, Tr2RenderContextEnum::PixelFormat format );
	bool Create2DArray( unsigned width, unsigned height, unsigned mipCount, unsigned arraySize, Tr2RenderContextEnum::PixelFormat format );
	bool CreateCube( unsigned width, unsigned mipCount, Tr2RenderContextEnum::PixelFormat format );
	bool CreateVolume( unsigned width, unsigned height, unsigned depth, unsigned mipCount, Tr2RenderContextEnum::PixelFormat format );
	bool CreateFromBitmapDimensions( const Tr2BitmapDimensions& dimensions );

	unsigned GetPitch() const;

	const char* GetRawData() const;
	char* GetRawData();
	const char* GetRawData( unsigned x, unsigned y ) const;
	char* GetRawData( unsigned x, unsigned y );
	size_t GetRawDataSize() const;
	size_t GetArrayElementSize() const;

	const char* GetMipRawData( unsigned level, uint32_t arrayIndex = 0 ) const;
	char* GetMipRawData( unsigned level, uint32_t arrayIndex = 0 );

	bool PopulateMargin( unsigned margin );

	bool Downsample2x2();
	bool Crop( unsigned left, unsigned top, unsigned right, unsigned bottom );

	bool ConvertToVolume();

	bool ChangeFormat( Tr2RenderContextEnum::PixelFormat format );

	bool GenerateMipMaps();
	bool ConvertFormat( Tr2RenderContextEnum::PixelFormat format );

protected:
	bool CheckForMatch( const Tr2BitmapDimensions& bd, bool checkDimensions, bool& alphaConvert, const char* log );

	std::string m_name;

	CcpMallocBuffer m_data;
private:
	bool GenerateMipLevel( uint8_t* source, unsigned width, unsigned height, uint8_t* destination );
};

}

#endif