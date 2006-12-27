
#include "StdAfx.h"
#include "becher.h"
#include "troll.h"
#include "obj_shop.h"

static CVar v_cost("shop_cost", 200, TVAR_SAVE); // cena za stavbu
static CVar v_cost_wood("shop_cost_wood", 100, TVAR_SAVE); // pocet dreva potrebneho na stavbu
static CVar v_cost_stone("shop_cost_stone", 50, TVAR_SAVE); // pocet kameni potrebneho na stavbu

/////////////////////////////////////////////////////////////
Shop::Shop(IHoeScene * scn) : BecherBuilding(scn)
{
	SetModel((IHoeModel*)GetResMgr()->ReqResource(model_SHOP));
	SetRingParam(1.5f,3.f,2.f);
}

bool Shop::Save(BecherGameSave &w)
{
	BecherBuilding::Save(w);
	w.WriteReservedWords(10);
	return true;
}

bool Shop::Load(BecherGameLoad &r)
{
	BecherBuilding::Load(r);
	r.ReadReservedWords(10);
	OnUpdateSur();
	return true;
}

#ifndef BECHER_EDITOR
ResourceBase * Shop::GetResource(ESurType type)
{
	switch (type)
	{
	default:
		return NULL;
	};
}



void Shop::Update(const float t)
{
}

bool Shop::InsertSur(ESurType type, uint *s)
{
	return false;
}

bool Shop::SetToWork(Troll * t)
{
	return false;
}

void Shop::UnsetFromWork(Troll * t)
{
}

bool Shop::Select()
{
	BecherBuilding::Select();
	GetLua()->func("s_shop");
	return true;
}

bool Shop::Idiot(TJob * t)
{
	return false;
}


#else
bool Shop::Select()
{
	BecherBuilding::Select();
	return true;
}

void Shop::OnChangeProp(int id, const HoeEditor::PropItem & pi)
{
}

#endif


