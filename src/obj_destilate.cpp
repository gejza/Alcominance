
#include "StdAfx.h"
#include "becher.h"
#include "troll.h"
#include "obj_destilate.h"
#include "obj_construct.h"

static CVar v_numzpr("dest_speed", 1.f, TVAR_SAVE); // rychlost zpracovani
static CVar v_sklad("dest_max", 500, TVAR_SAVE); // max. velikost miniskladu
static CVar v_cost("dest_cost", 180, TVAR_SAVE); // cena za stavbu
static CVar v_cost_wood("dest_cost_wood", 40, TVAR_SAVE); // pocet dreva potrebneho na stavbu
static CVar v_cost_stone("dest_cost_stone", 30, TVAR_SAVE); // pocet kameni potrebneho na stavbu
static CVar v_numworks("dest_maxwork", 4, TVAR_SAVE); // maximalni pocet pracujicich
static CVar v_recept("dest_recept", "S1=1", TVAR_SAVE); // recept pro jednu davku
static CVar v_coalmax("dest_coal_max", 120, TVAR_SAVE); // maximalni kapacita pro uhli


////////////////////////////////////////////////////////
Destilate::Destilate(IHoeScene * scn) : FactoryBuilding(scn), m_alco(EBS_Alco)
{
	SetModel((IHoeModel*)GetResMgr()->ReqResource(model_DESTILATE));
	SetRingParam(4.5f, 4.5f, 2.f);
	//GetCtrl()->SetFlags(HOF_SHOW);
	//m_mode = wmIn;
	m_sugar.SetOwner(this);
	m_w.SetOwner(this);
	m_alco.SetOwner(this); 

	m_part.emitor = (IHoeParticleEmitor*)GetEngine()->Create("particle");
	m_part.pos.Set(4.f,13.f,-14.f);
	GetCtrl()->Link(THoeSubObject::Particle, &m_part);

    m_wrk_sugar = 0;    
    m_wrk_coal = 0;

}

Destilate::~Destilate()
{
}

bool Destilate::Save(BecherGameSave &w)
{
	BecherBuilding::Save(w);
	w.WriteReservedWords(10);
	return true;
}

bool Destilate::Load(BecherGameLoad &r)
{
	BecherBuilding::Load(r);
	r.ReadReservedWords(10);
	OnUpdateSur();
	return true;
}

int Destilate::GetInfo(int type, char * str, size_t n)
{
	register int ret = 0;
	if (type==BINFO_Custom && str)
	{
		if (strcmp(str, "alco") == 0)
			type = BINFO_NumAlco;
		else if (strcmp(str, "sugar") == 0)
			type = BINFO_NumSugar;
	}
	switch (type)
	{
	case BINFO_NumAlco:
		ret = (int)this->m_alco.GetNum();
		if (str)
			_snprintf(str, n, "%d", ret);
		return ret;
	case BINFO_NumSugar:
		ret = (int)this->m_sugar.GetNum();
		if (str)
			_snprintf(str, n, "%d", ret);
		return ret;
	default:
		return BecherBuilding::GetInfo(type, str, n);
	};
	return 0;
}

int Destilate::GameMsg(int msg, int par1, void * par2, uint npar2)
{
	switch (msg)
	{
	case BMSG_Select:
		Select();
		break;
	case BMSG_SelectPlace:
	case BMSG_StartBuilding:
		return BuildPlace((float*)par2, 
			(IHoeModel*)GetResMgr()->ReqResource(model_DESTILATE),50.f,200.f,msg==BMSG_StartBuilding);
	}
	return BecherBuilding::GameMsg(msg, par1, par2, npar2);
}

void Destilate::SetMode(EBuildingMode mode)
{
	// odmazat
	if (mode == m_mode)
		return;
	// pri buildingu nastavit kolize
	switch (m_mode)
	{
	case EBM_Build:
		GetCtrl()->SetOverColor(0xffffffff);
		break;
	case EBM_Select:
		GetCtrl()->SetOverColor(0xffffffff);
		break;
	case EBM_Normal:
		CRR::Get()->Unregister(&m_alco);
		break;
	};
	m_mode = mode;
	switch (mode)
	{
	case EBM_Build:
		GetCtrl()->SetOverColor(0xffffff00);
		break;
	case EBM_Normal:
		Show(true);
		CRR::Get()->Register(&m_alco);
		break;
	};
}

#ifndef BECHER_EDITOR

bool Destilate::InsertSur(ESurType type, uint *s)
{
	if (type==EBS_Sugar)
	// max
		return m_sugar.Add(s, v_sklad.GetInt() - GetMiniStoreCount());
	else
		return false;
}

bool Destilate::SetToWork(Troll * t)
{
    switch (t->GetJob().type){
    case TJob::jtWork:
	    if (m_worked.Count() >= (uint)v_numworks.GetInt()) return false;
	    m_worked.Add(t);	
        break;
    case TJob::jtGotoRes:
        switch (t->GetJob().surtype){
        case EBS_Sugar:
            m_wrk_sugar++;
            break;        
        case EBS_Coal:
            m_wrk_coal++;
            break;
        }
        break;
    }
    return true;
}

void Destilate::UnsetFromWork(Troll * t)
{
	switch(t->GetJob().type){
    case TJob::jtWork:
	    m_worked.Remove(t);
        break;
    case TJob::jtGotoRes:
        switch(t->GetJob().surtype){
        case EBS_Sugar:
            m_wrk_sugar--;
            break;
        case EBS_Coal:
            m_wrk_coal--;
            break;
        }
        break;
    }
}

void Destilate::Update(const float t)
{
	// update
	float prog = m_w.InProcess() ? m_worked.Count()*v_numzpr.GetFloat():0.f;

	if (m_worked.Count() > 0)
	{
		m_w.Update(t*prog);

		if (m_w.CanOut() && ((int)m_w.Out(false)<=(v_sklad.GetInt() - GetMiniStoreCount())))
		{
			uint p = m_w.Out(true);
			m_alco.Add(&p, p);
		}

		// naplneni
		if (m_w.CanIn() && m_w.In(&m_sugar, 'S', true))
		{
			m_w.ToProcess();
		}
	}

	if (m_progress != prog)
	{
		// update 
		m_progress = prog;
		// pokud neni progress a nemuze se delat
		if (m_progress > 0.f)
			m_part.emitor->Start();
		else
			m_part.emitor->Stop();
	}

	if (m_worked.Count() > 0)
	{
		if (prog > 0.f)
			m_exitdelay.Reset();
		else if (m_exitdelay.AddTime((const float)t, m_worked.Count() == 1 ? 3.f:1.f))
		{
			m_exitdelay.Reset();
					// propustit jednoho workera
			m_worked.OneStopWork();
		}
	}

}


bool Destilate::Select()
{
	FactoryBuilding::Select();
	GetLevel()->GetPanel()->SetObjectHud("scripts/alco.menu",this);
	GetLua()->func("s_lihovar");
	return true;
}

bool Destilate::Idiot(TJob * j)
{
	// zjistit pripadny zdroj pro suroviny
	// 
	// navalit informace do tabulky, bud z crr nebo primo vybrane uloziste
	ResourceExp * ri = CRR::Get()->Find(EBS_Sugar, this);
    ResourceExp * rc = CRR::Get()->Find(EBS_Coal, this);
	// odnaseni!
	
	HoeGame::LuaFunc f(GetLua(), "i_alco");
	f.PushTable();
	
	f.SetTableInteger("max_store", v_sklad.GetInt());

    // vstupni suroviny
	f.SetTableInteger("sugar_avail", ri ? ri->GetAvail():0);
    f.SetTableInteger("sugar_wrkcount", m_wrk_sugar);
    f.SetTableInteger("sugar", m_sugar.GetNum());

    f.SetTableInteger("coal_avail", rc ? rc->GetAvail():0);
    f.SetTableInteger("coal_wrkcount", m_wrk_coal);
	f.SetTableInteger("coal", m_w.GetNum());
    f.SetTableInteger("coal_max", v_coalmax.GetInt());

	// works
    f.SetTableInteger("works_count", this->m_worked.Count());    
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
		j->type = TJob::jtGotoRes;
		j->num = f.GetTableInteger("num", -1); // pocet k prineseni
		j->from = ri;
		break;
	case 1:
		j->type = TJob::jtGotoWork;
		break;
	};
		
	f.Pop(1);
	
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





