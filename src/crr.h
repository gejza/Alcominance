#ifndef _CRR_H_
#define _CRR_H_

#include "id.h"

class BecherObject;

class ResourceBase
{
protected:
	uint m_actual;
public:
	ResourceBase() { m_actual = 0; }
	inline uint GetNum() const { return (int)m_actual; }
	inline void SetNum(uint num) { m_actual = num; }
	bool Add(uint *s, int max);
	/**
	* Vybiraci funkce
	* @param p Zda brat i jen cast
	*/
	uint Get(uint req, bool p);
};

class ResourceImp : public ResourceBase
{
public:
};

enum ESurPriority
{
	// nevydavat
	// spis zanechat
	// normalni
	// zbavit se
    EBSP_None = 0,
    EBSP_TimeWork,
};

class ResourceExp : public ResourceBase
{
protected:
	ESurType m_type;
	uint m_max;
	ESurPriority m_priority;
	BecherObject * m_owner;
public:
	ResourceExp(ESurType type);
	void SetOwner(BecherObject * own) { m_owner = own; }
	inline BecherObject * GetOwner() { assert(m_owner); return m_owner; }
	inline ESurType GetType() { return m_type; }
	inline ESurPriority GetPriority() { return m_priority; }
	inline void SetPriority(ESurPriority p) { m_priority = p; }
	void Register();
	void Unregister();
};

/**
 * Central Register of Resource
 * Centralni registr zdroju (surovin) slouzi ...
 */
class CRR 
{
private:
    static CRR* this_;
	HoeGame::PtrSet<ResourceExp*> m_items[EBS_Max];
public:
    CRR();
    ~CRR();
	static CRR * Get();
    void Register(ResourceExp* item);
	/**
	* Najit podle priority, obsahu, vzdalenosti
	*/
	ResourceExp * Find(ESurType type);
};


#endif