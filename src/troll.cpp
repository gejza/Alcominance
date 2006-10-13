
#include "StdAfx.h"
#include "becher.h"
#include "buildings.h"
#include "crr.h"
#include "troll.h"
#include "phys_prov.h"

static CVar v_speed("troll_speed", 35.f, TVAR_SAVE);
static CVar v_num("troll_num", 10, TVAR_SAVE);
static CVar v_cost_work("troll_cost_work", 0.1f, TVAR_SAVE);
static CVar v_cost_bring("troll_cost_bring", 0.1f, TVAR_SAVE);
static CVar v_cost_wait("troll_cost_wait", 0.1f, TVAR_SAVE);
static const float scale = 0.35f;

Troll::Troll(IHoeScene * scn) : BecherObject(scn)
{
	SetModel((IHoeModel*)GetResMgr()->ReqResource(model_TROLL));
	GetCtrl()->SetScale(HoeMath::Vector3(scale,scale,scale));
	GetCtrl()->SetFlags(HOF_SHOW|HOF_UPDATE|HOF_SCALED);
	SetRingParam(.8f,.8f,2.f);
	memset(&m_job, 0, sizeof(m_job));
	m_job.type = TJob::jtNone;
	m_load.locked = false;
	m_load.numsur = 0;
	m_load.surtype = EBS_None;

	anim = 0.f;
	m_nextfind = 0.f;
}

Troll::~Troll()
{
}

bool Troll::Save(BecherGameSave &w)
{
	BecherObject::Save(w);
	// ulozit job a path
	this->m_job.Save(w);
	w.WriteValue<bool>(m_load.locked);
	w.WriteValue<uint>(m_load.numlocked);
	w.WriteValue<uint>(m_load.numsur);
	w.WriteValue<dword>(m_load.surtype);
	// path
	w.WriteValue<bool>(false);
	return true;
}

bool Troll::Load(BecherGameLoad &r)
{
	BecherObject::Load(r);
	m_job.Load(r);	
	m_load.locked = r.Read<bool>();
	m_load.numlocked = r.Read<uint>();
	m_load.numsur = r.Read<uint>();
	m_load.surtype = (ESurType)r.Read<dword>();
	// path
	r.Read<bool>();
	return true;
}

void Troll::Update(const float t)
{
#ifndef BECHER_EDITOR
	// pokud chodi tak chodi
	// pokud ceka, tak nic
	switch (m_job.type)
	{
	case TJob::jtFly:
		// jde pryc
		anim += ((t * v_speed.GetFloat()) / (scale * 20.f)) ;
		if (anim > 1.f)
			anim = 0.f;
		this->SetAnimationTime(anim);
		if (m_path.Step(this, (float)t*v_speed.GetFloat()))
			Finish();
		break;
	case TJob::jtGotoRes:
	case TJob::jtGotoOwnerWithRes:
	case TJob::jtGotoWork:
		// update cesty, pokud cesta hotova tak finish
		anim += ((t * v_speed.GetFloat()) / (scale * 20.f)) ;
		if (anim > 1.f)
			anim = 0.f;
		this->SetAnimationTime(anim);
		if (m_path.Step(this, (float)t*v_speed.GetFloat()))
			Finish();
		// prachy
		GetLevel()->GetMJobs()->AddPay(v_cost_bring.GetFloat() * t);
		break;
	case TJob::jtFindJob:
		//if (m_nextfind > 0.f)
		//	m_nextfind -= t;
		//else
		//{
			FindJob(m_job.owner);
		//	m_nextfind = 10.f;
		//}
		GetLevel()->GetMJobs()->AddPay(v_cost_wait.GetFloat() * t);
		break;
	case TJob::jtWork:
		GetLevel()->GetMJobs()->AddPay(v_cost_work.GetFloat() * t);
		break;
	};
#endif
}

void Troll::SetJob(const TJob & j)
{
	assert(j.owner != NULL);
	// opustit stary job
	switch (m_job.type)
	{
	case TJob::jtWork:
	case TJob::jtGotoRes:
		if (j.type != TJob::jtFly)
			break;
    case TJob::jtGotoOwnerWithRes:
		assert(m_job.owner);
		m_job.owner->UnsetFromWork(this);
		break;
	};

	// nastavit novy
	if (m_load.locked)
	{
		m_job.from->Unlock(m_load.numlocked);
		m_load.numlocked = 0;
		m_load.locked = false;
	}

	m_job = j;
	// nastavit parametry
	switch (j.type)
	{
	case TJob::jtGotoRes:
		if (m_job.num > v_num.GetInt())
			m_job.num = v_num.GetInt();
		if (m_job.from->GetPriority() != EBSP_TimeWork)
		{ 
			m_load.numlocked = m_job.from->Lock(m_job.num);
			m_load.locked = true;
		}
		m_path.FindPath(this, m_job.from->GetOwner());
   		m_job.owner->SetToWork(this);
		break;
	case TJob::jtGotoOwnerWithRes:
		if (!m_job.to)
			m_job.to = m_job.owner;
		m_path.FindPath(this, m_job.to);
		break;
	case TJob::jtGotoWork:
		m_path.FindPath(this,m_job.owner);
		break;
	case TJob::jtWork:
		assert(m_job.owner);
		m_job.owner->SetToWork(this);
		break;
	case TJob::jtFly:
		// nastavit na padaka

		break;
	};
}

void Troll::Finish()
{
	switch (m_job.type)
	{
	case TJob::jtGotoRes:
		if (m_job.from->GetPriority() == EBSP_TimeWork)
		{
			assert(m_load.locked == false);
			dynamic_cast<SourceBuilding*>(m_job.from->GetOwner())->SetToGet(this, m_job.num);
			// nastavit job na cekani
			TJob j = m_job;
			j.type = TJob::jtWaitToRes;
			SetJob(j);
		}
		else
		{
			// pokud jde pro surovinu, tak vyjmout a nastavit
			assert(m_load.locked == true);
			m_job.from->Unlock(m_load.numlocked);
			m_load.locked = false;
			m_load.surtype = m_job.surtype;
			m_load.numsur = m_job.from->Get(m_load.numlocked,true);
			// nastavit na chuzi k zpatky
			TJob j = m_job;
			j.type = TJob::jtGotoOwnerWithRes;
			SetJob(j);
		}
		break;
	case TJob::jtGotoOwnerWithRes:
		// vlozit do budovy a hledat novy job
// odevzdat surovinu, mozna by mohl cekat dokud nebude volno ve skladu
		assert(m_job.to);
		m_job.to->InsertSur(m_load.surtype, &m_load.numsur);
		m_load.numsur = 0;
		m_load.surtype = EBS_None;
		FindJob(m_job.owner);
		break;
	case TJob::jtGotoWork:
		{
			//this->ToBuilding();
			TJob j = m_job;
			j.type = TJob::jtWork;
			SetJob(j);
		}
		break;
	};
}

void Troll::SurIn(ESurType type, uint num)
{
	//assert(type == m_job.surtype);
	//assert(m_phase == WaitForSur);
	//m_numsur = num;
	//m_surtype = type;
	// prenastavit job
	m_load.surtype = type;
	m_load.numsur = num;
	TJob j = m_job;
	j.type = TJob::jtGotoOwnerWithRes;
	SetJob(j);
}

bool Troll::FindJob(BecherBuilding * pref)
{
	TJob job;
	memset(&job,0,sizeof(job));
	//assert(pref);
	if (pref->Idiot(&job))
	{
		SetJob(job);
		return true;
	}
	else
	{
		TJob j = m_job;
		j.type = TJob::jtFindJob;
		j.owner = pref;
		SetJob(j);
	}
	return false;
}

void Troll::StopWork()
{
	// nastavit job na none
	// hledat novy job
	// pokud nenalezen, zajit k budove
	TJob j = m_job;
	j.type = TJob::jtFindJob;
	SetJob(j);
}

#ifndef BECHER_EDITOR
bool Troll::Select()
{
	BecherObject::Select();
	GetLua()->func("s_tupoun");
	return true;
}
#else
bool Troll::Select()
{
	BecherObject::Select();
	return true;
}

#endif

////////////////////////////////////////////////
/*JobEx::JobEx()
{
	SetNone();
	phase = JobEx::GoToNew;
}

const Job & JobEx::operator = (const Job & j)
{
	this->type = j.type;
	this->owner = j.owner;
	this->num = j.num;
	this->surtype = j.surtype;
	this->phase = JobEx::GoTo;
	switch (j.type)
	{
	case Job::jtPrines:
		//path.SetPosTo(j.store);
		break;
	case Job::jtWork:
		path.SetPosTo(j.owner);
		break;
	};
	return *this;
}

void JobEx::SetNone()
{
	type = jtNone;
}*/

/////////////////////////////////////////
// Path
bool Path::GetNextPos(float l,float &px, float &py)
{
	float ux = x - px;
	float uy = y - py;
	float mag = sqrt(ux * ux + uy * uy);
	ux /= mag;
	uy /= mag;
	if (l < mag)
	{
		px += ux * l;
		py += uy * l;
		return false;
	}
	px = x;
	py = y;

	// dalsi to
	act++;
	if (act >= this->m_points.Count())
		return true;
	x = m_points[act].x;
	y = m_points[act].y;

	return false;
}

bool Path::FindPath(float fx, float fy, float tx, float ty)
{
	x = ty;y = ty;
	// vymazat 
	// najit 
	HoeMath::Vector2 from,to;
	from.x = fx;
	from.y = fy;
	to.x = tx;
	to.y = ty;
	TPathPart * p = Phys::Get()->Find(from, to);
	if (p)
	{
		m_points.SetCount(0);
		p->Copy(this);
		assert(m_points.Count()>0);
		act = 0;
		x = m_points[0].x;
		y = m_points[0].y;
		delete p;
	}
	else
		return false;
	return true;
}

bool Path::FindPath(BecherObject * from, BecherObject * to)
{
	return FindPath(from->GetPosX(), from->GetPosY(), to->GetPosX(), to->GetPosY());
}

/*void Path::SetPosTo(float X, float Y)
{
}

void Path::SetPosTo(BecherObject * bo)
{
	SetPosTo( bo->GetPosX(), bo->GetPosY());
}*/

bool Path::Step(Troll * t, const float time)
{
	bool finish;
	float posX = t->GetPosX();
	float posY = t->GetPosY();
	float puvX = posX;
	float puvY = posY;
	finish = GetNextPos(time, posX,posY);
	float angle = -atan2f(-posX*10.f+puvX*10.f,-posY*10.f+puvY*10.f);
	t->SetAngle(angle);
	// nastavit pozici podle terenu
	t->SetPosition( posX, posY, GetLevel()->GetScene()->GetScenePhysics()->GetHeight(posX,posY));
	return finish;
}


