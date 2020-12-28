#include <stdexcept>
#include "symbol_table.h"

PymSymRef::PymSymRef(PymSymbol* p, TreeNode* rN)
	: parent(p), refNode(rN)
{
}

PymSymbol::PymSymbol(SymbolTable* p, TreeNode* dN)
	: parent(p), declNode(dN)
{
}

int SymbolTable::internal_id = 0;

SymbolTable::SymbolTable(TreeNode* t)
	: tableNode(t), id(internal_id++), lower(0), next(0),nextAttachPoint(&lower)
{

}

SymbolTable::SymbolTable(TreeNode* t, int id)
	: tableNode(t), id(id), lower(0), next(0),nextAttachPoint(&lower)
{
	SymbolTable::internal_id = id;
}

PymSymbol& SymbolTable::lookup(const std::string& name)
{
	auto it = hashTable.find(name);
	if (it == hashTable.end())
	{
		if (upper.expired())
		{
			throw std::invalid_argument("Symbol " + name + " not found!");
		}
		else
		{
			return std::shared_ptr<SymbolTable>(upper)->lookup(name);
		}
	}
	return it->second;
}

bool SymbolTable::check_local(const std::string& name)
{
	return hashTable.find(name) != hashTable.end();
}

std::ostream& operator<<(std::ostream& os, const SymbolTable& st)
{
	return os;
}
