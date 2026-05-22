// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "MemoryStream.h"


ReadMemoryStream::ReadMemoryStream( const void* memory, size_t size )
	:m_memory( memory ),
	m_size( size ),
	m_position( 0 )
{
}

ptrdiff_t ReadMemoryStream::Read( void* dest, ptrdiff_t count )
{
	if( !m_memory )
	{
		return -1;
	}
	if( count < 0 )
	{
		count = m_size - m_position;
	}
	count = std::min( count, ptrdiff_t( m_size - m_position ) );
	memcpy( dest, static_cast<const uint8_t*>( m_memory ) + m_position, count );
	m_position += count;
	return count;
}

ptrdiff_t ReadMemoryStream::Write( const void* source, size_t count )
{
	return -1;
}

ptrdiff_t ReadMemoryStream::Seek( ptrdiff_t distance, SeekOrigin method )
{
	if( !m_memory )
	{
		return -1;
	}
	switch( method )
	{
	case SO_BEGIN:
		m_position = size_t( std::min( ptrdiff_t( m_size ), std::max( distance, ptrdiff_t( 0 ) ) ) );
		break;
	case SO_CURRENT:
		m_position = std::min( size_t( std::max( ptrdiff_t( m_position ) + distance, ptrdiff_t( 0 ) ) ), m_size );
		break;
	case SO_END:
		m_position = m_size - std::min( size_t( std::max( distance, ptrdiff_t( 0 ) ) ), m_size );
		break;
	default:
		return -1;
	}
	return GetPosition();
}

ptrdiff_t ReadMemoryStream::GetPosition()
{
	if( !m_memory )
	{
		return -1;
	}
	return m_position;
}

ptrdiff_t ReadMemoryStream::GetSize()
{
	if( !m_memory )
	{
		return -1;
	}
	return m_size;
}


WriteMemoryStream::WriteMemoryStream( size_t maxSize )
	:m_maxSize( maxSize ),
	m_position( 0 )
{
}

ptrdiff_t WriteMemoryStream::Read( void* dest, ptrdiff_t count )
{
	return -1;
}

ptrdiff_t WriteMemoryStream::Write( const void* source, size_t count )
{
	if( m_maxSize == 0 )
	{
		return -1;
	}
	if( m_position + count > m_memory.size() )
	{
		size_t newSize = std::min( m_position + count, m_maxSize );
		m_memory.resize( "WriteMemoryStream::m_memory", newSize );
	}
	count = std::min( size_t( m_memory.size() - m_position ), count );
	memcpy( m_memory.get() + m_position, source, count );
	m_position += count;
	return count;
}

ptrdiff_t WriteMemoryStream::Seek( ptrdiff_t distance, SeekOrigin method )
{
	if( !m_maxSize )
	{
		return -1;
	}
	switch( method )
	{
	case SO_BEGIN:
		m_position = size_t( std::min( ptrdiff_t( m_memory.size() ), std::max( distance, ptrdiff_t( 0 ) ) ) );
		break;
	case SO_CURRENT:
		m_position = std::min( size_t( std::max( ptrdiff_t( m_position ) + distance, ptrdiff_t( 0 ) ) ), m_memory.size() );
		break;
	case SO_END:
		m_position = m_memory.size() - std::min( size_t( std::max( distance, ptrdiff_t( 0 ) ) ), m_memory.size() );
		break;
	default:
		return -1;
	}
	return GetPosition();
}

ptrdiff_t WriteMemoryStream::GetPosition()
{
	if( !m_maxSize )
	{
		return -1;
	}
	return m_position;
}

ptrdiff_t WriteMemoryStream::GetSize()
{
	if( !m_maxSize )
	{
		return -1;
	}
	return m_memory.size();
}

const void* WriteMemoryStream::GetData() const
{
	return m_memory.get();
}

size_t WriteMemoryStream::GetDataSize() const
{
	return m_memory.size();
}
