#pragma once
#include "Config/Config.h"
#include "UI/Draw.h"
#include "Utils/Logging.h"

class c_context
{
public:
	c_context() : m_logger(std::make_shared<c_logger>()), m_draw(std::make_shared<c_draw>())
	{
	}

	~c_context()
	{
	}

	//Settings
	std::shared_ptr<c_logger> m_logger;

	/*Drawing*/
	std::shared_ptr<c_draw> m_draw;

	/*Settings, normal settings*/
	c_settings m_settings;
};

extern c_context ctx;
