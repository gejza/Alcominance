
#include "StdAfx.h"
#include "becher.h"
#include "game.h"
#include "troll.h"
#include "obj_destilate.h"
#include "obj_store.h"

static CVar v_numzpr("dest_work", 1.f, 0); // rychlost zpracovani
static HoeGame::CTimer t_numzpr(v_numzpr);
static CVar v_sklad("dest_max", 5000, 0); // velikost miniskladu, trtina muze zabirat max 95%
static CVar v_numworks("dest_maxwork", 4, 0);

DestilateStatic Destilate::m_userhud;

DestilateStatic::DestilateStatic()
{
	m_act = NULL;
}

void DestilateStatic::SetAct(Destilate * act)
{
	m_act = act;
	// pripojit 
	dynamic_cast<HoeGame::Font*>(ReqItem("cukr"))->SetPtr(m_sugarinfo);
	dynamic_cast<HoeGame::Font*>(ReqItem("lih"))->SetPtr(m_alcoinfo);

}

void DestilateStatic::Draw(IHoe2D * h2d)
{
	if (m_act)
	{
		sprintf(m_sugarinfo,"%d cukru.", m_act->m_sugar.GetNum());
		sprintf(m_alcoinfo,"%d lihu.", m_act->m_alco.GetNum());

		ObjectHud::Draw(h2d);
	}
}

////////////////////////////////////////////////////////
Destilate::Destilate(IHoeScene * scn) : FactoryBuilding(scn), m_alco(EBS_Alco)
{
	SetModel((IHoeModel*)GetResMgr()->ReqResource(ID_DESTILATE));
	m_infoselect.s_x = 4.5f;
	m_infoselect.t_y = 2.f;
	m_infoselect.s_z = 4.5f;
	//GetCtrl()->SetFlags(HOF_SHOW);
	//m_mode = wmIn;
	m_alco.SetOwner(this); CRR::Get()->Register(&m_alco);
}

Destilate::~Destilate()
{
}

#ifndef BECHER_EDITOR

bool Destilate::InsertSur(ESurType type, uint *s)
{
	assert(type==EBS_Sugar);
	if (type==EBS_Sugar)
	// max
		return m_sugar.Add(s, v_sklad.GetInt() - GetMiniStoreCount());
	else
		return false;
}

bool Destilate::SetToWork(Troll * t)
{
	if (m_worked.Count() >= (uint)v_numworks.GetInt())
		return false;
	m_worked.Add(t);
	return true;
}

void Destilate::UnsetFromWork(Troll * t)
{
	m_worked.Remove(t);
}

void Destilate::Update(const double t)
{
	if (m_worked.Count() > 0)
	{
		if (m_sugar.GetNum() > 0)
		{
			uint p = t_numzpr.Compute(t * m_worked.Count());
			p=m_sugar.Get(p,true);
			m_alco.Add(&p, v_sklad.GetInt() - GetMiniStoreCount());
			m_exitdelay.Reset();
		}
		else
		{
			// postupne propoustet
			if (m_exitdelay.AddTime(t, 3.f))
			{
				m_exitdelay.Reset();
				// propustit jednoho workera
				m_worked.OneStopWork();
			}
		}
	}
}


bool Destilate::Select()
{
	FactoryBuilding::Select();
	GetLevel()->SetObjectHud(&m_userhud);
	m_userhud.SetAct(this);
	if (!IsBuildMode())
        GetLua()->func("s_lihovar");
	return true;
}

#else

bool Destilate::Select()
{
	FactoryBuilding::Select();
	/*GetProp()->Begin(this);
	GetProp()->AppendCategory(_("Store"));
	GetProp()->AppendLong(6, _("Limit"), v_sklad.GetInt());
	GetProp()->End();*/	
	return true;
}

void Destilate::OnChangeProp(int id, const HoeEditor::PropItem & pi)
{
	/*switch (id)
	{
	case 6:
		v_sklad.Set((int)pi.GetLong());
		break;
	};*/
}

#endif

bool Destilate::Idiot(Job * j)
{
	// zjistit pripadny zdroj pro suroviny
	// 
	// navalit informace do tabulky, bud z crr nebo primo vybrane uloziste
	ResourceExp * ri = CRR::Get()->Find(EBS_Sugar); // urceni priorit
	
	HoeGame::LuaFunc f(GetLua(), "i_alco");
	f.PushTable();
	// suroviny
	// informace o surovinach
	f.SetTableInteger("max_store", v_sklad.GetInt());
	f.SetTableInteger("sugar_avail", ri ? ri->GetNum():0);
	f.SetTableInteger("sugar", m_sugar.GetNum());
	f.SetTableInteger("alco", m_alco.GetNum());
	// works
	f.SetTableInteger("works", this->m_worked.Count());
	f.SetTableInteger("works_max", v_numworks.GetInt());
	f.Run(1);
	if (f.IsNil(-1))
	{
		f.Pop(1);
		return false;
	}

	// prevest zpatky na job
	int r = f.GetTableInteger("type", -1); // typ prace
	j->percent = f.GetTableFloat("percent", -1); // na kolik procent je vyzadovano
	j->owner = this;
	switch (r)
	{
	case 0:
		j->surtype = (ESurType)f.GetTableInteger("sur", -1); // typ suroviny
		j->type = Job::jtPrines;
		j->num = f.GetTableInteger("num", -1); // pocet k prineseni
		j->ritem = ri;
		break;
	case 1:
		j->type = Job::jtWork;
		break;
	};
		
	f.Pop(1);
	
	return true;
}



