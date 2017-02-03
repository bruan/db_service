#pragma once
#include "db_service_base.h"

#include <map>
#include <vector>
#include <memory>

#include "google\protobuf\message.h"

namespace db
{
	class CDbCache
	{
	public:
		CDbCache();
		~CDbCache();

		std::shared_ptr<google::protobuf::Message> 
				getData(uint32_t nDataID);
		void	setData(uint32_t nDataID, std::shared_ptr<google::protobuf::Message>& pData);
		void	delData(uint32_t nDataID);
		
	private:
		std::map<uint32_t, std::shared_ptr<google::protobuf::Message>>	m_mapCache;
	};
}
