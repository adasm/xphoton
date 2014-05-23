#include "stdafx.h"
#include "Scene.h"



Scene::Scene()
{
	m_pBIHLeafs = 0;
	m_pBIH = new rtBIH;

	m_pLightBIHLeafs = 0;
	m_pLightBIH = new rtBIH;
}

Scene::~Scene()
{
	/*
	delete m_pBIH;
	if (m_pBIHLeafs)
		_aligned_free(m_pBIHLeafs);

	delete m_pLightBIH;
	if (m_pLightBIHLeafs)
		_aligned_free(m_pLightBIHLeafs);
		*/
}



void Scene::load(const char* pFileName)
{
	vertex_buffer.clear();
	face_buffer.clear();

	ModelOBJ m_Model;
	m_Model.import(pFileName, true);

	//default material
	Material mat;
	mat.diffuse_color = XMFLOAT3(1, 1, 1);
	mat.reflection_color = XMFLOAT3(0, 0, 0);
	mat.flags = MAT_DIFFUSE;
	material_buffer.push_back(mat);

	std::map<const ModelOBJ::Material*, u32> m_MaterialsMap;
	for (u32 i = 0; i<m_Model.getNumberOfMaterials(); i++)
	{
		const ModelOBJ::Material& objMaterial = m_Model.getMaterial(i);
		m_MaterialsMap[&objMaterial] = i+1;

		Material mat;
		mat.diffuse_color = XMFLOAT3(objMaterial.diffuse);
		mat.reflection_color = XMFLOAT3(objMaterial.specular);
		mat.emission_color = XMFLOAT3(objMaterial.ambient);
		mat.glossy_factor = objMaterial.shininess;
		mat.flags = 0;

		//if (mat.diffuse_color.x>0 || mat.diffuse_color.y>0 || mat.diffuse_color.z>0)
			mat.flags |= MAT_DIFFUSE;

		if (mat.reflection_color.x>0 || mat.reflection_color.y>0 || mat.reflection_color.z>0)
			mat.flags |= MAT_REFLECTION;

		material_buffer.push_back(mat);
	}


	vertex_buffer.resize(m_Model.getNumberOfVertices());
	for (int i = 0; i<m_Model.getNumberOfVertices(); i++)
	{
		vertex_buffer[i].position = float3(m_Model.getVertexBuffer()[i].position[0],
										 m_Model.getVertexBuffer()[i].position[1],
										 m_Model.getVertexBuffer()[i].position[2]);

		vertex_buffer[i].normal = float3(m_Model.getVertexBuffer()[i].normal[0],
										 m_Model.getVertexBuffer()[i].normal[1],
										 m_Model.getVertexBuffer()[i].normal[2]);

		vertex_buffer[i].tangent = float3(m_Model.getVertexBuffer()[i].tangent[0],
										 m_Model.getVertexBuffer()[i].tangent[1],
										 m_Model.getVertexBuffer()[i].tangent[2]);
	}

	Face face;
	srand(200);
	u32 triCount = 0;

	face_buffer.resize(m_Model.getNumberOfTriangles());
	for (int i = 0; i<m_Model.getNumberOfMeshes(); i++)
	{
		ModelOBJ::Mesh m_Mesh = m_Model.getMesh(i);

		for (int j = 0; j<m_Mesh.triangleCount; j++)
		{
			//face.color = float4((float)(rand()%1000)/1000.0f, (float)(rand()%1000)/1000.0f, (float)(rand()%1000)/1000.0f, 1);
			face.i0 = m_Model.getIndexBuffer()[m_Mesh.startIndex + 3*j    ];
			face.i1 = m_Model.getIndexBuffer()[m_Mesh.startIndex + 3*j + 1];
			face.i2 = m_Model.getIndexBuffer()[m_Mesh.startIndex + 3*j + 2];
			face.matIndex = m_Mesh.pMaterial ? m_MaterialsMap[m_Mesh.pMaterial] : 0;

			face_buffer[triCount] = face;
			triCount++;
		}
	}

	// build Bounding Interval Hierarchy

	m_pBIHLeafs = (rtBIHLeaf*)_aligned_realloc(m_pBIHLeafs, sizeof(rtBIHLeaf) * face_buffer.size(), 16);

	for (u32 i = 0; i<face_buffer.size(); i++)
	{
		vector v0 = XMLoadFloat3(&vertex_buffer[face_buffer[i].i0].position);
		vector v1 = XMLoadFloat3(&vertex_buffer[face_buffer[i].i1].position);
		vector v2 = XMLoadFloat3(&vertex_buffer[face_buffer[i].i2].position);

		m_pBIHLeafs[i].box.createTriAABB(v0, v1, v2);
		m_pBIHLeafs[i].pUserData = (void*)i;
	}

	m_pBIH->build(m_pBIHLeafs, face_buffer.size(), SURFACE_AREA);

	buildBihBuffer(m_pBIH->m_pRoot);

}


void Scene::buildBihBuffer(rtBIHNode* pNode)
{
	BIHNode node;
	XMStoreFloat3(&node.box_min, pNode->m_BoundingBox.Min);
	XMStoreFloat3(&node.box_max, pNode->m_BoundingBox.Max);


	rtBIHNode* pCurrNode = pNode;
	while (true)
	{
		if (pCurrNode->m_pParent == 0)
		{
			node.failureJump = -1;
			break;
		}

		if (&(pCurrNode->m_pParent->m_pChild[0]) == pCurrNode)
		{
			//jump to "brother"
			node.failureJump = pCurrNode->m_pParent->m_pChild[1].m_ID;
			break;
		}

		pCurrNode = pCurrNode->m_pParent;
	}


	if (pNode->hasLeaves())
	{
		node.successJump = node.failureJump;


		node.count = pNode->getLeavesCount();

		for (u32 i = 0; i<node.count; i++)
			node.index[i] = (u32)(pNode->m_pLeaves[i]->pUserData);

		bih_buffer.push_back(node);

	}
	else
	{
		node.successJump = pNode->m_pChild[0].m_ID;
		node.count = 0;

		bih_buffer.push_back(node);

		buildBihBuffer(&(pNode->m_pChild[0]));
		buildBihBuffer(&(pNode->m_pChild[1]));
	}
}


void Scene::buildLightsBih()
{
	m_pLightBIHLeafs = (rtBIHLeaf*)_aligned_realloc(m_pLightBIHLeafs, sizeof(rtBIHLeaf) * light_buffer.size(), 16);

	for (u32 i = 0; i<light_buffer.size(); i++)
	{
		m_pLightBIHLeafs[i].box.Min = XMVectorSet(light_buffer[i].pos.x - light_buffer[i].radius,
											 light_buffer[i].pos.y - light_buffer[i].radius,
											 light_buffer[i].pos.z - light_buffer[i].radius, 0.0f);

		m_pLightBIHLeafs[i].box.Max = XMVectorSet(light_buffer[i].pos.x + light_buffer[i].radius,
											 light_buffer[i].pos.y + light_buffer[i].radius,
											 light_buffer[i].pos.z + light_buffer[i].radius, 0.0f);

		m_pLightBIHLeafs[i].pUserData = (void*)i;
	}

	m_pLightBIH->build(m_pLightBIHLeafs, light_buffer.size(), VOLUME);
	buildLightBihBuffer(m_pLightBIH->m_pRoot);
}

void Scene::buildLightBihBuffer(rtBIHNode* pNode)
{
	BIHNode node;
	XMStoreFloat3(&node.box_min, pNode->m_BoundingBox.Min);
	XMStoreFloat3(&node.box_max, pNode->m_BoundingBox.Max);


	rtBIHNode* pCurrNode = pNode;
	while (true)
	{
		if (pCurrNode->m_pParent == 0)
		{
			node.failureJump = -1;
			break;
		}

		if (&(pCurrNode->m_pParent->m_pChild[0]) == pCurrNode)
		{
			//jump to "brother"
			node.failureJump = pCurrNode->m_pParent->m_pChild[1].m_ID;
			break;
		}

		pCurrNode = pCurrNode->m_pParent;
	}


	if (pNode->hasLeaves())
	{
		node.successJump = node.failureJump;


		node.count = pNode->getLeavesCount();

		for (u32 i = 0; i<node.count; i++)
			node.index[i] = (u32)(pNode->m_pLeaves[i]->pUserData);

		lightBih_buffer.push_back(node);

	}
	else
	{
		node.successJump = pNode->m_pChild[0].m_ID;
		node.count = 0;

		lightBih_buffer.push_back(node);

		buildLightBihBuffer(&(pNode->m_pChild[0]));
		buildLightBihBuffer(&(pNode->m_pChild[1]));
	}
}