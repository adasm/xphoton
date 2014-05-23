#pragma once

#include "Math.h"
#include "MemPool.h"

struct rtBIHLeaf
{
	void* pUserData;
	Box box;
};

#define BIH_AXIS_X (0x0)
#define BIH_AXIS_Y (0x1)
#define BIH_AXIS_Z (0x2)
#define BIH_LEAF (0x3)

enum rtHeuristics
{
	SURFACE_AREA = 0,
	VOLUME,
};


struct rtNodeBuildDesc
{
	rtHeuristics heuristics;

	//temporary buffers
	Box* LeftAABBs;
	Box* RightAABBs;
	rtBIHLeaf** pSortedLeaves[3];

	//source data info
	rtBIHLeaf** pSrcLeaves;
	UINT leavesCount;
	UINT sortedBy;

	rtOneWayMemPool* pLeavesMemPool;
	rtOneWayMemPool* pNodesMemPool;
};

class rtBIHNode
{
private:
	friend class rtBIH;

	void buildSubNodes(rtNodeBuildDesc* pDesc, UINT* pCounter);

	void insertSortObjectsByAxis(rtBIHLeaf** ppArray, const int Start, const int End, const UINT Axis);
	void quickSortObjectsByAxis(rtBIHLeaf** ppArray, const int Start, const int End, const UINT Axis);

public:
	UINT m_ID;
	UINT m_Flags;
	rtBIHNode* m_pParent;
	union
	{
		rtBIHNode* m_pChild;
		rtBIHLeaf** m_pLeaves;
	};
	Box m_BoundingBox;

	rtBIHNode();
	~rtBIHNode();
	void* operator new (size_t);
	void operator delete(void* ptr);

	inline UINT getLeavesCount() { return m_Flags >> 2; };
	inline bool hasLeaves() { return ((m_Flags & 0x3) == 0x3); };

	void calcBox();
	void clear();
};


class rtBIH
{
public:
	rtBIHNode* m_pRoot;
	rtOneWayMemPool* m_pLeavesMemPool;
	rtOneWayMemPool* m_pNodesMemPool;

	rtBIH();
	~rtBIH();

	void build(rtBIHLeaf *pLeafs, const UINT leavesCount, rtHeuristics heuristics);
	void clear();
};

extern LARGE_INTEGER g_sortingTime;