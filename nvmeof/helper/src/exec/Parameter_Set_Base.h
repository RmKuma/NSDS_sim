#ifndef PARAMETER_SET_BASE_H
#define PARAMETER_SET_BASE_H

#include "ns3/rapidxml.hpp"
#include "ns3/XMLWriter.h"

class Parameter_Set_Base
{
public:
	virtual void XML_serialize(Utils::XmlWriter& xmlwriter) = 0;
	virtual void XML_deserialize(rapidxml::xml_node<> *node) = 0;
};

#endif // !PARAMETER_SET_BASE_H