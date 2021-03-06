
#include "../StdAfx.h"
#include "../becher.h"
#include "editor_tools.h"
#include "../buildings.h"
#include "editor_map.h"
#include "editor.h"
#include "../sysobjs.h"
#include "../terrain.h"
#include "../elements.h"

void BaseEditorTool::RightUp(const int x, const int y, const wxMouseEvent &e)
{
	BecherEdit::Get()->OnDefaultTool();
}
//////////////////////////////////////////////////////
// ToolSelect
ToolSelect::ToolSelect()
{
}

void ToolSelect::LeftDown(const int x, const int y, const wxMouseEvent &e)
{
	if (e.ControlDown())
	{
		//m_map->m_lockobject = m_map->GetObject(x,y);
	}
	else
	{
		assert(BecherEdit::Get()->GetActMap());
		BecherEdit::Get()->GetActMap()->SelectObject(x,y);
	}
}

//////////////////////////////////////////////////////
// ToolCreateObject
float ToolCreateObject::m_rand_scale_from = 0.5f;
float ToolCreateObject::m_rand_scale_to = 2.4f;

ToolCreateObject::ToolCreateObject(EObjType type, bool repeat, bool randori)
{
	m_type = type;
	m_repeat = repeat;
	m_rand_angle = m_rand_scale = randori;
	// vytvorit m_obj
	if (type < EBO_Max)
	{
	    m_obj = BecherEdit::Get()->GetActMap()->CreateObject(type);
        m_obj->Show(false);
        if (randori)
	    {
		    m_obj->SetAngle((rand() % 628) * 0.01f);
	    }
	}
	else if (type < EBAO_Max)
	{
		m_obj = BecherEdit::Get()->GetActMap()->CreateAddOnObject(type);
        m_obj->Show(false);
        if (randori)
	    {
		    m_obj->SetAngle((rand() % 628) * 0.01f);
	    }
	}
	else
	{
		// pouze v editoru je zvuk jako becherobject (kvuli zobrazovani)
		m_obj = BecherEdit::Get()->GetActMap()->CreateSystemObject(type);
		m_obj->Show(false);
	}

	GetProp()->Begin(this);
	GetProp()->AppendCategory(_("Generic"));
	GetProp()->AppendBool(1, _("Repeat"), m_repeat);
	GetProp()->AppendCategory(_("Random Values"));
	GetProp()->AppendBool(2, _("Angle"), m_rand_angle);
	GetProp()->AppendBool(3, _("Scale"), m_rand_scale);
	GetProp()->AppendFloat(4, _("Scale From"), m_rand_scale_from);
	GetProp()->AppendFloat(5, _("Scale To"), m_rand_scale_to);
	GetProp()->End();
}

ToolCreateObject::~ToolCreateObject()
{
	// if 
	if (GetProp()->GetHandler() == static_cast<HoeEditor::PropObject*>(this))
		GetProp()->Begin(NULL);
	SAFE_DELETE(m_obj);
}

void ToolCreateObject::SetPos(int absX, int absY)
{
	float sx,sy;
	BecherEdit::Get()->GetActMap()->GetView()->GetPick(absX,absY,&sx,&sy);

	m_obj->SetPosition(sx,sy,BecherEdit::Get()->GetActMap()->GetScene()->GetScenePhysics()->GetHeight(sx,sy)); 
}

void ToolCreateObject::Enter(int absX, int absY)
{
	m_obj->Show(true);
	SetPos(absX, absY);
}

void ToolCreateObject::Leave()
{
	m_obj->Show(false);
}

void ToolCreateObject::Move(int relX, int relY, int absX, int absY, const wxMouseEvent &ev)
{
	/*float sx,sy;
	if (m_map && m_map->GetCreatedObject())
	{
		
		if (m_map->GetView()->GetPick(absX,absY,&sx,&sy))
			m_map->GetCreatedObject()->SetPosition(sx,sy); 
	}
	else if (m_map && m_map->m_lockobject)
	{
		if (m_map->GetView()->GetPick(absX,absY,&sx,&sy))
			m_map->m_lockobject->SetPosition(sx,sy); 

	}*/
	SetPos( absX, absY);
}

void ToolCreateObject::LeftUp(const int x, const int y, const wxMouseEvent &e)
{
	SetPos(x,y);
	if (m_type < EBO_Max)
		BecherEdit::Get()->GetActMap()->AddObject(m_obj);
	else if (m_type < EBAO_Max)
		BecherEdit::Get()->GetActMap()->AddAddonObject(reinterpret_cast<Addon*>(m_obj));
	else
		BecherEdit::Get()->GetActMap()->AddSystemObject(reinterpret_cast<BecherSystemObject*>(m_obj));

	if (m_repeat || e.ControlDown())
	{
		if (m_type < EBO_Max)
			m_obj = BecherEdit::Get()->GetActMap()->CreateObject(m_type);
		else if (m_type < EBAO_Max)
		{
			Addon * ao = BecherEdit::Get()->GetActMap()->CreateAddOnObject(m_type);
			if (m_rand_scale)
				ao->SetScale(HoeCore::RandFloat(m_rand_scale_from, m_rand_scale_to));
			m_obj = ao;
		}
		else
			m_obj = BecherEdit::Get()->GetActMap()->CreateSystemObject(m_type);
		m_obj->Show(true);
		SetPos(x,y);
		if (m_rand_angle)
		{
			m_obj->SetAngle((rand() % 628) * 0.01f);
		}
	}
	else
	{
		m_obj = NULL;
		BecherEdit::Get()->OnDefaultTool();
	}

	// undo action
	//BecherEdit::Get()->AddUndo(new UndoCreate());
}

void ToolCreateObject::Wheel( const wxMouseEvent &e)
{
    if (e.ControlDown())
        m_obj->SetAngle(m_obj->GetAngle() + e.GetWheelRotation() / 500.f);
}

void ToolCreateObject::OnChangeProp(int id, const HoeEditor::PropItem & pi)
{
	switch (id)
	{
	case 1:
		m_repeat = pi.GetBool();
		break;
	case 2: // angle
		//v_sklad.Set((int)pi.GetLong());
		m_rand_angle = pi.GetBool();
		break;
	case 3: // scale
		m_rand_scale = pi.GetBool();
		break;
	case 4: // scale
		m_rand_scale_from = pi.GetFloat();
		break;
	case 5: // scale
		m_rand_scale_to = pi.GetFloat();
		break;
	};
}

//////////////////////////////////////////////////////
// ToolTex
ToolTex::ToolTex(byte tex)
{
	m_tex = tex;
	m_leftdown = false;
	m_nx = -10;
	m_ny = -10;
}

void ToolTex::LeftDown(const int x, const int y, const wxMouseEvent &e)
{
	float sx,sy;
	m_leftdown = true;
	if (BecherEdit::Get()->GetActMap()->GetView()->GetPick(x,y,&sx,&sy))
	{
		EditorMap & m = *BecherEdit::Get()->GetActMap();
		// 300 / 20
		//const uint nx = (uint)((sx+(m.m_sizeX+m.m_distX)*0.5f)/(m.m_distX));
		//const uint ny = (uint)((sy+(m.m_sizeY+m.m_distY)*0.5f)/(m.m_distY));
		//const uint nx = (uint)((sx+(m.m_sizeX)*0.5f)/(m.m_distX));
		//const uint ny = (uint)((sy+(m.m_sizeY)*0.5f)/(m.m_distY));
		const uint nx = (uint)((sx+(m.m_sizeX)*0.5f)/(m.m_distX));
		const uint ny = (uint)((sy+(m.m_sizeY)*0.5f)/(m.m_distY));
		SetTerrainTexture(&m, nx+1, ny+1, m_tex);
		m_nx = nx;
		m_ny = ny;
	}
}

void ToolTex::LeftUp(const int x, const int y, const wxMouseEvent &e)
{
	m_leftdown = false;
}

/*void ToolTex::RightDown(const int x, const int y, const wxMouseEvent &e)
{
	BecherEdit::Get()->SetTool(NULL);
}*/

void ToolTex::Move(int relX, int relY, int absX, int absY, const wxMouseEvent & ev)
{
	float sx,sy;
	if (BecherEdit::Get()->GetActMap()->GetView()->GetPick(absX,absY,&sx,&sy))
	{
		EditorMap & m = *BecherEdit::Get()->GetActMap();
		const uint nx = (uint)((sx+m.m_distX*0.5f+(m.m_sizeX)*0.5f)/(m.m_distX));
		const uint ny = (uint)((sy+m.m_distY*0.5f+(m.m_sizeY)*0.5f)/(m.m_distY));
		wxString str;
		str.Printf("x: %f y: %f nx: %d ny: %d", sx, sy, nx, ny);
		BecherEdit::Get()->SetStatus(str);
		if (m_leftdown && m_nx != nx && m_ny != ny)
		{

			m_nx = nx; m_ny = ny;
			SetTerrainTexture(&m, nx+1, ny+1, m_tex);
		}
	}
}

void ToolTex::Wheel(const wxMouseEvent &e)
{
	float sx,sy;
	if (BecherEdit::Get()->GetActMap()->GetView()->GetPick(e.GetX(),e.GetY(),&sx,&sy))
	{
		EditorMap & m = *BecherEdit::Get()->GetActMap();
		//const uint nx = (uint)((sx+(m.m_sizeX+m.m_distX)*0.5f)/(m.m_distX));
		//const uint ny = (uint)((sy+(m.m_sizeY+m.m_distY)*0.5f)/(m.m_distY));
		const uint nx = (uint)((sx+(m.m_sizeX)*0.5f)/(m.m_distX));
		const uint ny = (uint)((sy+(m.m_sizeY)*0.5f)/(m.m_distY));
		IHoeEnv::GridSurface::TGridDesc desc;
		m.GetTerrain()->GetGridDesc(nx,ny,&desc);
		bool top = true;
		if (desc.tex2 == 0xff || e.ControlDown())
			top = false;
		
		int d = top ? desc.x2 * 4 + desc.y2:desc.x1 * 4 + desc.y1;
		d += e.GetWheelRotation() > 0 ? 31:1;
		d = d % 32;
		if (top)
		{
			desc.x2 = d / 4;
			desc.y2 = d % 4;
		}
		else
		{
			desc.x1 = d / 4;
			desc.y1 = d % 4;
		}
		m.GetTerrain()->SetGridDesc(nx,ny,&desc);
        m.GetTerrain()->Load();
	}

}

//////////////////////////////////////////////////////
// ToolTerrain
ToolTerrain::ToolTerrain(float size, float radius)
{
	m_size = size;
	m_radius = radius;
}

ToolTerrain::~ToolTerrain()
{
	BecherEdit::Get()->GetActMap()->ModelHeightUpdate();
}

void ToolTerrain::LeftDown(const int x, const int y, const wxMouseEvent &e)
{
	float sx,sy;
	if (BecherEdit::Get()->GetActMap()->GetView()->GetPick(x,y,&sx,&sy))
	{
		EditorMap & m = *BecherEdit::Get()->GetActMap();
		//const uint nx = (uint)((sx+(m.m_sizeX)*0.5f)/(m.m_distX));
		//const uint ny = (uint)((sy+(m.m_sizeY)*0.5f)/(m.m_distY));
		// nastavit spravne vysku

		//m.GetTerrain()->SetGridPlane(nx, ny, 0.f, -1,1,1,-1);
		m.GetTerrain()->MoveHeight(sx,sy,m_size, m_radius);
		m.GetTerrain()->Load();
	}
}

void ToolTerrain::LeftUp(const int x, const int y, const wxMouseEvent &e)
{
}

void ToolTerrain::Move(int relX, int relY, int absX, int absY, const wxMouseEvent & ev)
{
	float sx,sy;
	if (BecherEdit::Get()->GetActMap()->GetView()->GetPick(absX,absY,&sx,&sy))
	{
		EditorMap & m = *BecherEdit::Get()->GetActMap();
		const uint nx = (uint)((sx+(m.m_sizeX)*0.5f)/(m.m_distX));
		const uint ny = (uint)((sy+(m.m_sizeY)*0.5f)/(m.m_distY));
		wxString str;
		str.Printf("x: %f y: %f nx: %d ny: %d h:%f", sx, sy, nx, ny, m.GetScene()->GetScenePhysics()->GetHeight(sx,sy));
		BecherEdit::Get()->SetStatus(str);
	}
}

//////////////////////////////////////////////////////
// ToolTex
ToolTerrainExp::ToolTerrainExp()
{
}

ToolTerrainExp::~ToolTerrainExp()
{
	BecherEdit::Get()->GetActMap()->ModelHeightUpdate();
}

void ToolTerrainExp::LeftDown(const int x, const int y, const wxMouseEvent &e)
{
	float sx,sy;
	if (BecherEdit::Get()->GetActMap()->GetView()->GetPick(x,y,&sx,&sy))
	{
		EditorMap & m = *BecherEdit::Get()->GetActMap();
		// 300 / 20
		//const uint nx = (uint)((sx+(m.m_sizeX+m.m_distX)*0.5f)/(m.m_distX));
		//const uint ny = (uint)((sy+(m.m_sizeY+m.m_distY)*0.5f)/(m.m_distY));
		const uint nx = (uint)((sx+(m.m_sizeX)*0.5f)/(m.m_distX));
		const uint ny = (uint)((sy+(m.m_sizeY)*0.5f)/(m.m_distY));
		m.GetTerrain()->SetGridModel(nx,ny, 0.f, 0);
		IHoeEnv::GridSurface::TGridDesc desc;
		m.GetTerrain()->GetGridDesc(nx,ny,&desc);
		desc.tex2 = 10;
		desc.x2 = 0;
		desc.y2 = 0;
		m.GetTerrain()->SetGridDesc(nx,ny,&desc);
		m.GetTerrain()->Load();
	}
}

/*void ToolTerrainExp::RightDown(const int x, const int y, const wxMouseEvent &e)
{
	BecherEdit::Get()->SetTool(NULL);
}*/

void ToolTerrainExp::Move(int relX, int relY, int absX, int absY, const wxMouseEvent & ev)
{
	float sx,sy;
	if (BecherEdit::Get()->GetActMap()->GetView()->GetPick(absX,absY,&sx,&sy))
	{
		EditorMap & m = *BecherEdit::Get()->GetActMap();
		const uint nx = (uint)((sx+(m.m_sizeX)*0.5f)/(m.m_distX));
		const uint ny = (uint)((sy+(m.m_sizeY)*0.5f)/(m.m_distY));
		wxString str;
		str.Printf("x: %f y: %f nx: %d ny: %d", sx, sy, nx, ny);
		BecherEdit::Get()->SetStatus(str);
	}
}

void ToolTerrainExp::Wheel(const wxMouseEvent &e)
{
	float sx,sy;
	if (BecherEdit::Get()->GetActMap()->GetView()->GetPick(e.GetX(),e.GetY(),&sx,&sy))
	{
		EditorMap & m = *BecherEdit::Get()->GetActMap();
		//const uint nx = (uint)((sx+(m.m_sizeX+m.m_distX)*0.5f)/(m.m_distX));
		//const uint ny = (uint)((sy+(m.m_sizeY+m.m_distY)*0.5f)/(m.m_distY));
		const uint nx = (uint)((sx+(m.m_sizeX)*0.5f)/(m.m_distX));
		const uint ny = (uint)((sy+(m.m_sizeY)*0.5f)/(m.m_distY));
		int model = m.GetTerrain()->GetGridModel(nx,ny);
		if (model == -1)
			return;
		model += (e.GetWheelRotation() > 0 ? 11:1);
		model = model % 12;
		m.GetTerrain()->SetGridModel(nx,ny, 0.f, model);
        m.GetTerrain()->Load();
	}

}
