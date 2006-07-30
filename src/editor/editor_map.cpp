
#include "../StdAfx.h"
#include "../becher.h"
#include "../buildings.h"
//#include "elements.h"
#include "editor_map.h"
//#include "plugin_editlevel.h"
#include "../sysobjs.h"


void EditorMap::CreateNew(uint sizeX, uint sizeY)
{

	m_numX = sizeX;
	m_numY = sizeY;

	m_sizeX = m_numX * 20.f;
	m_sizeY = m_numY * 20.f;
	m_distX = m_sizeX / m_numX;
	m_distY = m_sizeY / m_numY;
	CreateScene();
	GetEngine()->SetActiveScene(m_scene);
	m_mapfilepath = wxT("");
	m_terrain = m_scene->GetSceneEnv()->CreateGridSurface();
	m_terrain->SetTexture(0, "trava", 4, 4);
	m_terrain->SetTexture(1, "ter_war3", 8, 4);
	m_terrain->SetTexture(2, "City_Dirt", 8, 4);
	m_terrain->SetTexture(3, "City_SquareTiles", 4, 4);
	m_terrain->SetTexture(4, "Cliff1", 4, 4);
	m_terrain->SetTexture(5, "Concrete", 4, 4);
	m_terrain->SetTexture(6, "MetalPlateCliff", 4, 4);
	m_terrain->SetTexture(7, "MetalPlateTiles", 4, 4);
	m_terrain->Create(m_sizeX, m_sizeY,m_numX,m_numY);

	// vytvorit
	for (uint x=0;x < m_numX;x++)
	for (uint y=0;y < m_numY;y++)
	{
		IHoeEnv::GridSurface::TGridDesc desc;
		m_terrain->GetGridDesc(x,y,&desc);		
		desc.x2 = 0;
		desc.y2 = 0;
		desc.tex2 = 0xff;
		m_terrain->SetGridDesc(x,y,&desc);
	}

	m_terrain->Load();
	// textures
}

bool EditorMap::LoadMap(const wxString &path)
{
	HoeEditor::EditorFile file;
	if (!file.OpenRead(path))
		return false;
	BecherGameLoad r(&file);
	CreateScene();
	GetEngine()->SetActiveScene(m_scene);
	m_terrain = m_scene->GetSceneEnv()->CreateGridSurface();
	m_terrain->SetTexture(0, "trava", 4, 4);
	m_terrain->SetTexture(1, "ter_war3", 8, 4);
	m_terrain->SetTexture(2, "City_Dirt", 8, 4);
	m_terrain->SetTexture(3, "City_SquareTiles", 4, 4);
	m_terrain->SetTexture(4, "Cliff1", 4, 4);
	m_terrain->SetTexture(5, "Concrete", 4, 4);
	m_terrain->SetTexture(6, "MetalPlateCliff", 4, 4);
	m_terrain->SetTexture(7, "MetalPlateTiles", 4, 4);
	return Load( r, false);
}

bool EditorMap::SaveMap(const wxString &path)
{
	// open file for write
	HoeEditor::EditorFile file;
	if (!file.OpenWrite(path))
	{
		wxLogMessage(_("Failed open file %s for writing."), path.c_str());
		return false;
	}

	BecherGameSave w(&file);

	// write nejakej header
	w.WriteChunk(ID_BECHERFILE, ID_BECHERVER);

	w.WriteChunk(ID_CHUNK('t','e','r','r'), 0);
	m_terrain->Dump(&w);
	w.WriteChunkEnd();

	// ulozit systemove objekty
	for (int i=0;i < m_numsysobj;i++)
	{
		// uklada se jen typ
		w.WriteChunk(ID_CHUNK('s','y','s','o'), 1);
		w.WriteValue<dword>(m_sysobj[i]->GetType());
		m_sysobj[i]->Save(w);
		w.WriteChunkEnd();
	}

	// objekty
	SaveAllObjects(w);

	w.WriteChunk(ID_CHUNK('e','n','d',' '), 0);

	return true;
}

bool EditorMap::SaveMap()
{
	assert(!m_mapfilepath.IsEmpty());
	return SaveMap(m_mapfilepath);
}

void EditorMap::SelectObject(const int x, const int y)
{
	BecherObject * o = GetObject(x,y);
	if (o)
		o->Select();
	else
		GetProp()->Begin(NULL);
}

void EditorMap::SetFilePath(const wxString &path)
{
	m_mapfilepath = path;
}

wxString EditorMap::GetFilePath()
{
	return this->m_mapfilepath;
}

wxString EditorMap::GetTitle()
{
	return this->m_mapfilepath;
}

void EditorMap::Resize(int top, int bottom, int left, int right)
{
	// ulozeni stareho
	IHoeEnv::GridSurface::TGridDesc * desc = new IHoeEnv::GridSurface::TGridDesc[m_numX*m_numY];
	for (uint y=0;y < m_numY;y++)
		for (uint x=0;x < m_numX;x++)
			m_terrain->GetGridDesc(x,y, desc + m_numX * y + x);
	
	const uint nx = left + right + m_numX;
	const uint ny = top + bottom + m_numY;
	const float posx = (nx - m_numX) * m_distX;
	const float posy = (ny - m_numY) * m_distY;
	m_sizeX += posx; m_sizeY += posy;

	m_terrain->Create(m_sizeX, m_sizeY, nx, ny);
	for (uint y=0;y < ny;y++)
		for (uint x=0;x < nx;x++)
		{
			const uint px = left + x;
			const uint py = top + y;
			if (px >= 0 && px < m_numX && py >= 0 && py < m_numY)
				m_terrain->SetGridDesc(x,y, desc + m_numX * py + px);
			else
			{
				IHoeEnv::GridSurface::TGridDesc d;
				memset(&d, 0, sizeof d);
				d.tex2 = 1;
				m_terrain->SetGridDesc(x,y, &d);
			}
		}
	m_numX = nx;
	m_numY = ny;
	delete [] desc;

	// posunout vsechny na spravne misto
	for (int i=0;i<GetNumObj();i++)
	{
		BecherObject & o = *GetObj(i);
		o.SetPosition(o.GetPosX()-posx*0.5f, o.GetPosY()-posy*0.5f,0);
	}
}

void EditorMap::ShowSystemObjects(bool show)
{
	// prolezt a zapnout, nebo vypnout
	for (int i=0;i<m_numsysobj;i++)
		m_sysobj[i]->Show(show);
}

void EditorMap::ShowObjects(bool show, bool wire)
{
	for (int i=0;i<m_numobj;i++)
	{
		m_obj[i]->Show(show);
		if (wire)
			m_obj[i]->GetCtrl()->SetFlags(HOF_WIRE);
		else
			m_obj[i]->GetCtrl()->UnsetFlags(HOF_WIRE);
	}
}




