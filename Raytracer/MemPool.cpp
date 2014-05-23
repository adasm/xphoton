#include "stdafx.h"
#include "MemPool.h"

rtOneWayMemPool::rtOneWayMemPool(UINT blockSize, bool aligned)
{
	m_blockSize = blockSize;
	m_Aligned = aligned;
}

rtOneWayMemPool::~rtOneWayMemPool()
{
	release();
}

void* rtOneWayMemPool::alloc(size_t size)
{
	//no blocks, create a new one
	if (m_Blocks.size() == 0)
	{
		rtMemPoolBlock newBlock;
		if (m_Aligned)
			newBlock.ptr = (char*)_aligned_malloc(m_blockSize, 16);
		else
			newBlock.ptr = (char*)malloc(m_blockSize);
		newBlock.used = size;
		m_Blocks.push_back(newBlock);

		return newBlock.ptr;
	}

	//not enough space in block, create a next one
	rtMemPoolBlock& block = m_Blocks[m_Blocks.size()-1];
	if (m_blockSize < size + block.used)
	{
		rtMemPoolBlock newBlock;
		if (m_Aligned)
			newBlock.ptr = (char*)_aligned_malloc(m_blockSize, 16);
		else
			newBlock.ptr = (char*)malloc(m_blockSize);
		newBlock.used = size;
		m_Blocks.push_back(newBlock);

		return newBlock.ptr;
	}

	void* ptr = (void*)(((size_t)block.ptr) + block.used);
	block.used += size;
	return ptr;
}

void rtOneWayMemPool::release()
{
	if (m_Aligned)
	{
		for (int i = 0; i<m_Blocks.size(); i++)
			_aligned_free(m_Blocks[i].ptr);
	}
	else
	{
		for (int i = 0; i<m_Blocks.size(); i++)
			free(m_Blocks[i].ptr);
	}

	m_Blocks.clear();
}