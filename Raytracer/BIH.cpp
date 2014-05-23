#include "stdafx.h"
#include "BIH.h"


#define INSERT_SORT_TRESHOLD (24)

LARGE_INTEGER g_sortingTime;

void* rtBIHNode::operator new(size_t size)
{
	rtBIHNode* pMemBlock = (rtBIHNode*)_aligned_malloc(size, 16);
	return pMemBlock;
}

void rtBIHNode::operator delete(void* ptr)
{
	_aligned_free(ptr);
}

rtBIHNode::rtBIHNode()
{
	m_Flags = BIH_LEAF;

	m_pChild = 0;
	m_pLeaves = 0;
}

rtBIHNode::~rtBIHNode()
{
	clear();
}

void rtBIHNode::clear()
{
	m_Flags = BIH_LEAF;
}

void rtBIHNode::calcBox()
{
	if (hasLeaves())
	{
		UINT leavesCount = getLeavesCount();
		if (leavesCount > 0)
		{
			m_BoundingBox = m_pLeaves[0]->box;

			for (UINT i = 1; i<leavesCount; i++)
			{
				m_BoundingBox.Min = XMVectorMin(m_BoundingBox.Min, m_pLeaves[i]->box.Min);
				m_BoundingBox.Max = XMVectorMax(m_BoundingBox.Max, m_pLeaves[i]->box.Max);
			}
		}
		else
		{
			m_BoundingBox.Min = g_XMZero;
			m_BoundingBox.Max = g_XMZero;
		}
	}
	else
	{
		m_pChild[0].calcBox();
		m_pChild[1].calcBox();

		m_BoundingBox.Min = XMVectorMin(m_pChild[0].m_BoundingBox.Min, m_pChild[1].m_BoundingBox.Min);
		m_BoundingBox.Max = XMVectorMax(m_pChild[0].m_BoundingBox.Max, m_pChild[1].m_BoundingBox.Max);
	}
}

inline void * memcpy_4(void * dst, void const * src, size_t len)
{
	long * plDst = (long *) dst;
	long const * plSrc = (long const *) src;

	while (len >= 4)
	{
		*plDst++ = *plSrc++;
		len -= 4;
	}

	return (dst);
} 

void rtBIHNode::buildSubNodes(rtNodeBuildDesc* pDesc, UINT* pCounter)
{
	size_t dataSize = sizeof(rtBIHLeaf*) * pDesc->leavesCount;

	if (pDesc->leavesCount < 3)
	{
		m_pLeaves = (rtBIHLeaf**)pDesc->pLeavesMemPool->alloc(dataSize);
		memcpy_4(m_pLeaves, pDesc->pSrcLeaves, dataSize);

		m_Flags = BIH_LEAF | (pDesc->leavesCount << 2);
		return;
	}

	LARGE_INTEGER start, stop;

	
	
	// sort leaves
	for (UINT i = 0; i<3; i++)
	{
		if (i != pDesc->sortedBy)
		{
			memcpy_4(pDesc->pSortedLeaves[i], pDesc->pSrcLeaves, dataSize);

			if (pDesc->leavesCount > INSERT_SORT_TRESHOLD+1)
				quickSortObjectsByAxis(pDesc->pSortedLeaves[i], 0, pDesc->leavesCount-1, i);
			else
				insertSortObjectsByAxis(pDesc->pSortedLeaves[i], 0, pDesc->leavesCount-1, i);
		}
	}

	if (pDesc->sortedBy < 3)
		memcpy_4(pDesc->pSortedLeaves[pDesc->sortedBy], pDesc->pSrcLeaves, dataSize);


	XMVECTOR leftCost, rightCost, totalCost;
	XMVECTOR bestCost = XMVectorReplicate(1.0e+30f);

	UINT bestAxis = 3;
	UINT bestSplitPos = 0;

	int i;

	QueryPerformanceCounter(&start);
	for (UINT axis = 0; axis<3; axis++)
	{
		//calculate possible left node aabbs
		pDesc->LeftAABBs[0] = pDesc->pSortedLeaves[axis][0]->box;
		XMVECTOR prevMin = pDesc->LeftAABBs[0].Min;
		XMVECTOR prevMax = pDesc->LeftAABBs[0].Max;
		for (i = 1; i<pDesc->leavesCount; i++)
		{
			prevMin = XMVectorMin(prevMin, pDesc->pSortedLeaves[axis][i]->box.Min);
			prevMax = XMVectorMax(prevMax, pDesc->pSortedLeaves[axis][i]->box.Max);
			pDesc->LeftAABBs[i].Min = prevMin;
			pDesc->LeftAABBs[i].Max = prevMax;
		}


		//calculate possible right node aabbs
		pDesc->RightAABBs[pDesc->leavesCount-1] = pDesc->pSortedLeaves[axis][pDesc->leavesCount-1]->box;
		prevMin = pDesc->RightAABBs[pDesc->leavesCount-1].Min;
		prevMax = pDesc->RightAABBs[pDesc->leavesCount-1].Max;
		for (i = pDesc->leavesCount-2; i>=0; i--)
		{
			prevMin = XMVectorMin(prevMin, pDesc->pSortedLeaves[axis][i]->box.Min);
			prevMax = XMVectorMax(prevMax, pDesc->pSortedLeaves[axis][i]->box.Max);
			pDesc->RightAABBs[i].Min = prevMin;
			pDesc->RightAABBs[i].Max = prevMax;
		}

		UINT splitpos;
		UINT max_splitpos = pDesc->leavesCount-1;
		XMVECTOR leftCount, rightcount;

		switch (pDesc->heuristics)
		{
			case SURFACE_AREA:
				for (splitpos = 2; splitpos<max_splitpos; splitpos++)
				{
					leftCost = pDesc->LeftAABBs[splitpos].getSurfaceArea();
					rightCost = pDesc->RightAABBs[splitpos+1].getSurfaceArea();
					leftCount = XMConvertVectorUIntToFloat(XMVectorReplicateInt(splitpos + 1), 0);
					rightcount = XMConvertVectorUIntToFloat(XMVectorReplicateInt(max_splitpos - splitpos), 0);
					totalCost = leftCost*leftCount + rightCost*rightcount;

					if (XMVector3Greater(bestCost, totalCost))
					{
						bestAxis = axis;
						bestSplitPos = splitpos;
						bestCost = totalCost;
					}
				}
				break;

	
			case VOLUME:
				for (splitpos = 2; splitpos<max_splitpos; splitpos++)
				{
					leftCost = pDesc->LeftAABBs[splitpos].getVolume();
					rightCost = pDesc->RightAABBs[splitpos+1].getVolume();
					leftCount = XMVectorReplicate((float)(splitpos + 1));
					rightcount = XMVectorReplicate((float)(max_splitpos - splitpos));
					totalCost = leftCost*leftCount + rightCost*rightcount;

					if (XMVector3Greater(bestCost, totalCost))
					{
						bestAxis = axis;
						bestSplitPos = splitpos;
						bestCost = totalCost;
					}
				}
				break;
		}
	}
	QueryPerformanceCounter(&stop);
	g_sortingTime.QuadPart += stop.QuadPart - start.QuadPart;



	if (bestAxis >= 3)
	{
		bestSplitPos = 1;
		bestAxis = 0;
		/*
		m_pLeaves = (rtBIHLeaf**)pDesc->pLeavesMemPool->alloc(dataSize);
		memcpy_4(m_pLeaves, pDesc->pSrcLeaves, dataSize);

		m_Flags = BIH_LEAF | (pDesc->leavesCount << 2);
		return;
		*/
	}


	//
	m_pChild = (rtBIHNode*)pDesc->pNodesMemPool->alloc(sizeof(rtBIHNode)*2);
	UINT childLeavesCount;

	rtNodeBuildDesc desc;
	desc.heuristics = pDesc->heuristics;
	desc.pLeavesMemPool = pDesc->pLeavesMemPool;
	desc.pNodesMemPool = pDesc->pNodesMemPool;
	desc.LeftAABBs = pDesc->LeftAABBs;
	desc.RightAABBs = pDesc->RightAABBs;
	desc.pSortedLeaves[0] = pDesc->pSortedLeaves[0];
	desc.pSortedLeaves[1] = pDesc->pSortedLeaves[1];
	desc.pSortedLeaves[2] = pDesc->pSortedLeaves[2];
	desc.sortedBy = bestAxis;

	//construct left node
	m_pChild[0] = rtBIHNode();
	m_pChild[0].m_ID = *pCounter;
	m_pChild[0].m_pParent = this;
	desc.leavesCount = bestSplitPos+1;
	desc.pSrcLeaves = pDesc->pSortedLeaves[bestAxis];
	(*pCounter)++;
	m_pChild[0].buildSubNodes(&desc, pCounter);

	//construct right node
	m_pChild[1] = rtBIHNode();
	m_pChild[1].m_ID = *pCounter;
	m_pChild[1].m_pParent = this;
	desc.leavesCount = pDesc->leavesCount - (bestSplitPos+1);
	desc.pSrcLeaves = &(pDesc->pSortedLeaves[bestAxis][bestSplitPos+1]);
	(*pCounter)++;
	m_pChild[1].buildSubNodes(&desc, pCounter);

	m_Flags = bestAxis;
}

void rtBIHNode::insertSortObjectsByAxis(rtBIHLeaf** ppArray, const int Start, const int End, const UINT Axis)
{
	rtBIHLeaf *Tmp;
	float dist;
	int i, j;

	for (i = Start+1; i <= End; i++)
	{
		Tmp = ppArray[i];
		dist = XMVectorGetByIndex(Tmp->box.Max, Axis);

		for (j = i-1; (j >= Start) && (XMVectorGetByIndex(ppArray[j]->box.Max, Axis) > dist); j--)
			ppArray[j+1] = ppArray[j];

		ppArray[j+1] = Tmp;
	}
}

void rtBIHNode::quickSortObjectsByAxis(rtBIHLeaf** ppArray, const int Start, const int End, const UINT Axis)
{
	int i = Start;
	int j = End;
	rtBIHLeaf *Tmp;

	float dist = XMVectorGetByIndex(ppArray[ (Start + End) / 2]->box.Max, Axis);

	do 
	{
        while (XMVectorGetByIndex(ppArray[i]->box.Max, Axis) < dist) i++;
        while (XMVectorGetByIndex(ppArray[j]->box.Max, Axis) > dist) j--;

        if (i<=j)
		{
            Tmp = ppArray[i];
            ppArray[i] = ppArray[j];
            ppArray[j] = Tmp;
            i++;
            j--;
		}
	} while (i <= j);

    if (Start<j) 
	{
		if (j-Start > INSERT_SORT_TRESHOLD)
			quickSortObjectsByAxis(ppArray, Start, j, Axis);
		else
			insertSortObjectsByAxis(ppArray, Start, j, Axis);
	}

    if (End>i)
	{
		if (End-i > INSERT_SORT_TRESHOLD)
			quickSortObjectsByAxis(ppArray, i, End, Axis);
		else
			insertSortObjectsByAxis(ppArray, i, End, Axis);
	}
}




rtBIH::rtBIH()
{
	m_pRoot = 0;
	m_pLeavesMemPool = 0;
	m_pNodesMemPool = 0;
}

rtBIH::~rtBIH()
{
	clear();
}


void rtBIH::build(rtBIHLeaf *pLeafs, const UINT leavesCount, rtHeuristics heuristics)
{
	clear();

	m_pRoot = new rtBIHNode;
	m_pRoot->m_ID = 0;
	m_pRoot->m_pParent = 0;
	m_pLeavesMemPool = new rtOneWayMemPool(sizeof(rtBIHLeaf*) * leavesCount, false);
	m_pNodesMemPool = new rtOneWayMemPool(sizeof(rtBIHNode) * (leavesCount+2), true);

	rtNodeBuildDesc desc;
	desc.heuristics = heuristics;
	desc.leavesCount = leavesCount;
	desc.pLeavesMemPool = m_pLeavesMemPool;
	desc.pNodesMemPool = m_pNodesMemPool;
	desc.sortedBy = 3;
	
	desc.LeftAABBs = (Box*)_aligned_malloc(leavesCount * sizeof(Box), 16);
	desc.RightAABBs = (Box*)_aligned_malloc(leavesCount * sizeof(Box), 16);

	//do this to malloc only once
	UINT arraySize = leavesCount * sizeof(void*);
	desc.pSortedLeaves[0] = (rtBIHLeaf**)malloc(3 * arraySize);
	desc.pSortedLeaves[1] = (rtBIHLeaf**)((long)(desc.pSortedLeaves[0]) + arraySize);
	desc.pSortedLeaves[2] = (rtBIHLeaf**)((long)(desc.pSortedLeaves[1]) + arraySize);

	rtBIHLeaf** ppLeaves = (rtBIHLeaf**)malloc(sizeof(rtBIHLeaf*) * leavesCount);
	for (UINT i = 0; i<leavesCount; i++)
		ppLeaves[i] = &(pLeafs[i]);
	desc.pSrcLeaves = ppLeaves;

	UINT counter = 1;
	m_pRoot->buildSubNodes(&desc, &counter);

	_aligned_free(desc.LeftAABBs);
	_aligned_free(desc.RightAABBs);
	free(desc.pSortedLeaves[0]);
	free(ppLeaves);

	m_pRoot->calcBox();
}

void rtBIH::clear()
{
	if (m_pRoot)
	{
		delete m_pRoot;
		m_pRoot = 0;
	}

	if (m_pLeavesMemPool)
	{
		delete m_pLeavesMemPool;
		m_pLeavesMemPool = 0;
	}

	if (m_pNodesMemPool)
	{
		delete m_pNodesMemPool;
		m_pNodesMemPool = 0;
	}
}