#pragma once


struct rtMemPoolBlock
{
	char* ptr;
	size_t used;
};

class rtOneWayMemPool
{
private:
	bool m_Aligned;
	size_t m_blockSize;
	std::vector<rtMemPoolBlock> m_Blocks;

public:

	rtOneWayMemPool(UINT blockSize, bool aligned);
	~rtOneWayMemPool();

	void* alloc(size_t size);
	void release();
};