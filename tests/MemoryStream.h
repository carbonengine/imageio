// Copyright © 2014 CCP ehf.

#pragma once
#ifndef MemoryStream_H
#define MemoryStream_H

class ReadMemoryStream: public ICcpStream
{
public:
	ReadMemoryStream( const void* memory, size_t size );

	virtual ptrdiff_t Read( void* dest, ptrdiff_t count );
	virtual ptrdiff_t Write( const void* source, size_t count );
	virtual ptrdiff_t Seek( ptrdiff_t distance, SeekOrigin method );
	virtual ptrdiff_t GetPosition();
	virtual ptrdiff_t GetSize();
private:
	const void* m_memory;
	size_t m_size;
	size_t m_position;
};

class WriteMemoryStream: public ICcpStream
{
public:
	WriteMemoryStream( size_t maxSize = std::numeric_limits<size_t>::max() );

	virtual ptrdiff_t Read( void* dest, ptrdiff_t count );
	virtual ptrdiff_t Write( const void* source, size_t count );
	virtual ptrdiff_t Seek( ptrdiff_t distance, SeekOrigin method );
	virtual ptrdiff_t GetPosition();
	virtual ptrdiff_t GetSize();

	const void* GetData() const;
	size_t GetDataSize() const;
private:
	CcpMallocBuffer m_memory;
	size_t m_maxSize;
	size_t m_position;
};

#endif