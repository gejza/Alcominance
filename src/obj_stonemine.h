
#ifndef _BECHER_STONEMINE_H_
#define _BECHER_STONEMINE_H_

#include "buildings.h"

class StoneMine;

#ifndef BECHER_EDITOR
class StoneMineStatic : public ObjectHud
{
protected:
	StoneMine * m_act;
public:
	StoneMineStatic();
	void SetAct(StoneMine * act);
	virtual void Draw(IHoe2D * h2d);
};
#endif // BECHER_EDITOR

class StoneMine : public SourceBuilding
{
	friend class BecherLevel;
	friend class StoneMineStatic;
protected:
	// panel
#ifndef BECHER_EDITOR
	static StoneMineStatic m_userhud;
#endif // BECHER_EDITOR
public:
	StoneMine(IHoeScene * scn);

	DECLARE_BUILDING(EBO_StoneMine)
};


#endif // _BECHER_STONEMINE_H_
